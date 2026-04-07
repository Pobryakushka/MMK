#include "MeasurementExporter.h"
#include "amsprotocol.h"
#include "WindShearCalculator.h"

#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QPageLayout>
#include <QTextDocument>
#include <QPainter>

#include "xlsxdocument.h"
#include "xlsxformat.h"
#include "xlsxchart.h"
#include "xlsxabstractsheet.h"

// ─────────────────────────────────────────────────────────────────────────────

static bool hasAnyData(const MeasurementSnapshot &snap)
{
    return snap.coordinatesValid
           || snap.surfaceMeteoValid
           || !snap.avgWind.isEmpty()
           || !snap.actualWind.isEmpty()
           || !snap.measuredWind.isEmpty()
           || !snap.windShear.isEmpty()
           || snap.meteo11Updated.valid
           || snap.meteo11Approximate.valid;
}

// ══════════════════════════════════════════════════════════════════════════════
// generate()
// ══════════════════════════════════════════════════════════════════════════════

QString MeasurementExporter::generate(const MeasurementSnapshot &snap,
                                      const ExportOptions       &opts,
                                      QString                   &errorMsg)
{
    errorMsg.clear();
    if (snap.recordId <= 0) {
        errorMsg = "Нет загруженных данных для экспорта.\n"
                   "Выберите дату и время с доступными измерениями.";
        return {};
    }
    if (!hasAnyData(snap)) {
        errorMsg = QString("Запись ID %1 от %2 не содержит данных.")
                       .arg(snap.recordId)
                       .arg(snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
        return {};
    }
    switch (opts.format) {
    case ExportOptions::TXT:  return generateTxt (snap, opts);
    case ExportOptions::CSV:  return generateCsv (snap, opts);
    case ExportOptions::JSON: return generateJson(snap, opts);
    default: break;
    }
    return {};
}

// ══════════════════════════════════════════════════════════════════════════════
// generatePdf()
// ══════════════════════════════════════════════════════════════════════════════

static QSize computeChartSize(const ExportOptions &opts)
{
    double pageW_mm = 0, pageH_mm = 0;
    switch (opts.pdfPageSize) {
    case QPageSize::A3: pageW_mm = 297; pageH_mm = 420; break;
    case QPageSize::Letter: pageW_mm = 216; pageH_mm = 279; break;
    default: pageW_mm = 210; pageH_mm = 297; break;
    }
    if (opts.pdfLandscape) qSwap(pageW_mm, pageH_mm);

    double contentW_mm = pageW_mm - 24.0;

    const double dpi = 96.0;
    int totalW_px = static_cast<int>(contentW_mm / 25.4 * dpi);

    int chartW = (totalW_px - 8) / 2;
    int chartH = static_cast<int>(chartW * 0.70);

    return QSize(qMax(chartW, 180), qMax(chartH, 126));
}

bool MeasurementExporter::generatePdf(const MeasurementSnapshot &snap,
                                      const ExportOptions       &opts,
                                      const QString             &filePath,
                                      QString                   &errorMsg)
{
    errorMsg.clear();
    if (snap.recordId <= 0 || !hasAnyData(snap)) {
        errorMsg = "Нет данных для формирования PDF.";
        return false;
    }


    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(opts.pdfPageSize));
    printer.setPageOrientation(opts.pdfLandscape
                                   ? QPageLayout::Landscape
                                   : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setDefaultStyleSheet(
        "body { font-family: Arial, sans-serif; font-size: 9pt; color: #111; }");

    if (opts.pdfCharts) {
        const QSize chartSz = computeChartSize(opts);
        for (auto it = snap.charts.constBegin(); it != snap.charts.constEnd(); ++it) {
            if (!it.value().isNull()) {
                QImage scaled = it.value().scaled(chartSz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                doc.addResource(QTextDocument::ImageResource,
                                QUrl("img://" + it.key()),
                                QVariant(it.value()));
            }
        }
    }

    doc.setHtml(buildHtmlReport(snap, opts));
    doc.print(&printer);

    if (printer.printerState() == QPrinter::Error) {
        errorMsg = "Ошибка записи PDF-файла: " + filePath;
        return false;
    }
    return true;

}

bool MeasurementExporter::generateXlsx(const MeasurementSnapshot &snap,
                                       const ExportOptions       &opts,
                                       const QString             &filePath,
                                       QString                   &errorMsg)
{
    errorMsg.clear();
    if (snap.recordId <= 0 || !hasAnyData(snap)) {
        errorMsg = "Нет данных для формирования XLSX.";
        return false;
    }


    QXlsx::Document xlsx;
    {
        QStringList sheets = xlsx.sheetNames();
        xlsx.renameSheet(sheets.isEmpty() ? "Sheet1" : sheets.first(), "Сводка");
    }

    // ── Форматы ──────────────────────────────────────────────────────────────

    QXlsx::Format fmtTitle;
    fmtTitle.setFontSize(12);
    fmtTitle.setFontBold(true);
    fmtTitle.setFontColor(QColor("#1565C0"));

    QXlsx::Format fmtHeader;
    fmtHeader.setFontBold(true);
    fmtHeader.setFontColor(QColor("#FFFFFF"));
    fmtHeader.setPatternBackgroundColor(QColor("#1976D2"));
    fmtHeader.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    fmtHeader.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format fmtCell;
    fmtCell.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    fmtCell.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format fmtCellAlt;
    fmtCellAlt.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    fmtCellAlt.setPatternBackgroundColor(QColor("#EEF4FB"));
    fmtCellAlt.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format fmtLabel;
    fmtLabel.setFontBold(true);

    QXlsx::Format fmtValue;

    // ── Лист 1: Сводка ───────────────────────────────────────────────────────

    xlsx.setColumnWidth(1, 40);
    xlsx.setColumnWidth(2, 30);

    auto write2 = [&](int row, const QString &label, const QString &value) {
        xlsx.write(row, 1, label, fmtLabel);
        xlsx.write(row, 2, value, fmtValue);
    };

    xlsx.write(1, 1, "Результаты измерений — Метеокомплекс", fmtTitle);
    int r = 3;
    write2(r++, "Запись ID",    QString::number(snap.recordId));
    write2(r++, "Дата/время",   snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    write2(r++, "Ном. станции", snap.stationNumber);
    r++;

    if (opts.includeCoordinates && snap.coordinatesValid) {
        xlsx.write(r++, 1, "Координаты станции", fmtTitle);
        write2(r++, "Широта",           latToStr(snap.latitude));
        write2(r++, "Долгота",          lonToStr(snap.longitude));
        write2(r++, "Высота над УМ, м", QString::number(snap.altitude,'f',1));
        r++;
    }
    if (opts.includeSurfaceMeteo && snap.surfaceMeteoValid) {
        xlsx.write(r++, 1, "Наземные метеоусловия", fmtTitle);
        write2(r++, "Давление, мм рт.ст.",  QString::number(snap.pressureHpa,     'f',1));
        write2(r++, "Температура, °C",       QString::number(snap.temperatureC,    'f',1));
        write2(r++, "Влажность, %",          QString::number(snap.humidityPct,     'f',1));
        write2(r++, "Направление ветра, °",  QString::number(snap.surfaceWindDir,  'f',0));
        write2(r++, "Скорость ветра, м/с",   QString::number(snap.surfaceWindSpeed,'f',1));
        r++;
    }

    // ── Вспомогательные функции для вставки графиков ─────────────────────────
    auto insertLineChart = [&](int anchorRow, int anchorCol,
                            int firstDataRow, int lastDataRow,
                            int colData, int /*colCategories*/,
                            const QString &title,
                            QXlsx::AbstractSheet *sheet) {
    QXlsx::Chart *chart = xlsx.insertChart(anchorRow, anchorCol, QSize(420, 300));
    chart->setChartType(QXlsx::Chart::CT_LineChart);
    chart->setChartTitle(title);
    // Значения (Y)
    chart->addSeries(
        QXlsx::CellRange(firstDataRow, colData, lastDataRow, colData),
        sheet);
    // Категории (X) — передаём как вторую серию; QXlsx использует первый
    // addSeries как значения, второй — как подписи при наличии заголовка
    // chart->addSeries(
    //     QXlsx::CellRange(firstDataRow, colCategories, lastDataRow, colCategories),
    //     sheet);
};

auto insertBarChart = [&](int anchorRow, int anchorCol,
                           int firstDataRow, int lastDataRow,
                           int colData, int /*colCategories*/,
                           const QString &title,
                           QXlsx::AbstractSheet *sheet) {
    QXlsx::Chart *chart = xlsx.insertChart(anchorRow, anchorCol, QSize(420, 300));
    chart->setChartType(QXlsx::Chart::CT_LineChart);
    chart->setChartTitle(title);
    chart->addSeries(
        QXlsx::CellRange(firstDataRow, colData, lastDataRow, colData),
        sheet);
    // chart->addSeries(
    //     QXlsx::CellRange(firstDataRow, colCategories, lastDataRow, colCategories),
    //     sheet);
};

// ── Профили ветра (общий шаблон) ─────────────────────────────────────────
// Структура листа:
//   строка 1: заголовок
//   строка 3: заголовки колонок
//   строки 4+: данные
//   col1=Высота, col2=Скорость, col3=Направление
//   col5+: встроенные графики

auto makeWindSheet = [&](const QString &sheetName,
                          const QString &titleStr,
                          const QVector<WindProfileData> &data)
{
    if (data.isEmpty()) return;

    xlsx.addSheet(sheetName);

    // Получаем указатель на текущий лист СРАЗУ после addSheet
    QXlsx::AbstractSheet *sheet = xlsx.currentSheet();

    xlsx.setColumnWidth(1, 14);
    xlsx.setColumnWidth(2, 16);
    xlsx.setColumnWidth(3, 16);

    xlsx.write(1, 1, titleStr, fmtTitle);
    xlsx.write(3, 1, "Высота, м",      fmtHeader);
    xlsx.write(3, 2, "Скорость, м/с",  fmtHeader);
    xlsx.write(3, 3, "Направление, °", fmtHeader);

    const int dataRow = 4;
    for (int i = 0; i < data.size(); ++i) {
        const WindProfileData &p = data[i];
        QXlsx::Format &fmt = ((i % 2) == 0) ? fmtCellAlt : fmtCell;
        xlsx.write(dataRow + i, 1, static_cast<double>(p.height),        fmt);
        xlsx.write(dataRow + i, 2, static_cast<double>(p.windSpeed),     fmt);
        xlsx.write(dataRow + i, 3, static_cast<double>(p.windDirection), fmt);
    }

    const int lastRow = dataRow + data.size() - 1;

    // График скорости (col2) vs высота (col1)
    insertLineChart(dataRow, 5, dataRow, lastRow, 2, 1,
                    titleStr + " — скорость, м/с", sheet);

    // График направления (col3) vs высота (col1)
    insertLineChart(dataRow + 18, 5, dataRow, lastRow, 3, 1,
                    titleStr + " — направление, °", sheet);
};

if (opts.includeAvgWind    && !snap.avgWind.isEmpty())
    makeWindSheet("Средний ветер",  "Средний ветер",        snap.avgWind);
if (opts.includeActualWind && !snap.actualWind.isEmpty())
    makeWindSheet("Действ. ветер",  "Действительный ветер", snap.actualWind);

// ── Измеренный ветер ──────────────────────────────────────────────────────

if (opts.includeMeasuredWind && !snap.measuredWind.isEmpty()) {
    xlsx.addSheet("Измер. ветер");
    QXlsx::AbstractSheet *sheet = xlsx.currentSheet();

    xlsx.setColumnWidth(1, 14);
    xlsx.setColumnWidth(2, 16);
    xlsx.setColumnWidth(3, 16);

    xlsx.write(1, 1, "Измеренный ветер", fmtTitle);
    xlsx.write(3, 1, "Высота, м",        fmtHeader);
    xlsx.write(3, 2, "Скорость, м/с",    fmtHeader);
    xlsx.write(3, 3, "Направление, °",   fmtHeader);

    const int dataRow = 4;
    for (int i = 0; i < snap.measuredWind.size(); ++i) {
        const MeasuredWindData &p = snap.measuredWind[i];
        QXlsx::Format &fmt = ((i % 2) == 0) ? fmtCellAlt : fmtCell;
        xlsx.write(dataRow + i, 1, static_cast<double>(p.height),        fmt);
        xlsx.write(dataRow + i, 2, static_cast<double>(p.windSpeed),     fmt);
        xlsx.write(dataRow + i, 3, static_cast<double>(p.windDirection), fmt);
    }

    const int lastRow = dataRow + snap.measuredWind.size() - 1;

    insertLineChart(dataRow,      5, dataRow, lastRow, 2, 1,
                    "Измеренный ветер — скорость, м/с", sheet);
    insertLineChart(dataRow + 18, 5, dataRow, lastRow, 3, 1,
                    "Измеренный ветер — направление, °", sheet);
}

// ── Сдвиг ветра ───────────────────────────────────────────────────────────

if (opts.includeWindShear && !snap.windShear.isEmpty()) {
    xlsx.addSheet("Сдвиг ветра");
    QXlsx::AbstractSheet *sheet = xlsx.currentSheet();

    xlsx.setColumnWidth(1, 14);
    xlsx.setColumnWidth(2, 22);
    xlsx.setColumnWidth(3, 24);
    xlsx.setColumnWidth(4, 14);

    xlsx.write(1, 1, "Сдвиг ветра", fmtTitle);
    xlsx.write(3, 1, "Высота, м",            fmtHeader);
    xlsx.write(3, 2, "Скорость, м/с/30м",     fmtHeader);
    xlsx.write(3, 3, "Изм. направления, °",   fmtHeader);
    xlsx.write(3, 4, "Уровень",               fmtHeader);

    QXlsx::Format fmtLow, fmtMed, fmtHigh, fmtCrit;
    fmtLow. setPatternBackgroundColor(QColor("#C8E6C9"));
    fmtMed. setPatternBackgroundColor(QColor("#FFF9C4"));
    fmtHigh.setPatternBackgroundColor(QColor("#FFCCBC"));
    fmtCrit.setPatternBackgroundColor(QColor("#FFCDD2"));
    for (auto *f : {&fmtLow, &fmtMed, &fmtHigh, &fmtCrit}) {
        f->setHorizontalAlignment(QXlsx::Format::AlignHCenter);
        f->setBorderStyle(QXlsx::Format::BorderThin);
    }

    const int dataRow = 4;
    for (int i = 0; i < snap.windShear.size(); ++i) {
        const WindShearData &s = snap.windShear[i];
        QXlsx::Format *sf = &fmtCell;
        if      (s.severityLevel == 1) sf = &fmtLow;
        else if (s.severityLevel == 2) sf = &fmtMed;
        else if (s.severityLevel == 3) sf = &fmtHigh;
        else if (s.severityLevel >= 4) sf = &fmtCrit;

        xlsx.write(dataRow + i, 1, static_cast<double>(s.height), *sf);
        xlsx.write(dataRow + i, 2, s.shearPer30m,                  *sf);
        xlsx.write(dataRow + i, 3, s.shearDirection,               *sf);
        xlsx.write(dataRow + i, 4,
                   WindShearCalculator::getSeverityText(s.severityLevel), *sf);
    }

    const int lastRow = dataRow + snap.windShear.size() - 1;

    insertBarChart(dataRow,      6, dataRow, lastRow, 2, 1,
                   "Сдвиг скорости ветра, м/с/30м", sheet);
    insertBarChart(dataRow + 18, 6, dataRow, lastRow, 3, 1,
                   "Изменение направления ветра, °", sheet);
}
// TODO: ОСТАНОВКА
// ── Метео-11 ──────────────────────────────────────────────────────────────

if (opts.includeMeteo11Updated || opts.includeMeteo11Approx ||
    opts.includeMeteo11Station)
{
    xlsx.addSheet("Метео-11");
    xlsx.setColumnWidth(1, 36);
    xlsx.setColumnWidth(2, 50);

    int row = 1;
    auto writeM11 = [&](const MeasurementSnapshot::Meteo11Export &m,
                         const QString &label) {
        if (!m.valid) return;
        xlsx.write(row++, 1, label,                   fmtTitle);
        xlsx.write(row,   1, "Бюллетень",              fmtLabel);
        xlsx.write(row++, 2, m.bulletinString,         fmtValue);
        xlsx.write(row,   1, "Номер ст. (NNNNN)",      fmtLabel);
        xlsx.write(row++, 2, m.stationNumber,          fmtValue);
        xlsx.write(row,   1, "Высота ст. (BBBB)",      fmtLabel);
        xlsx.write(row++, 2, m.stationAltitude,        fmtValue);
        xlsx.write(row,   1, "Откл. давл. (БББ)",      fmtLabel);
        xlsx.write(row++, 2, m.pressureDev,            fmtValue);
        xlsx.write(row,   1, "Откл. т-ры (ТТ)",       fmtLabel);
        xlsx.write(row++, 2, m.tempVirtDev,            fmtValue);
        xlsx.write(row,   1, "Выс. темп. зонд., км",   fmtLabel);
        xlsx.write(row++, 2, m.reachedTempKm,          fmtValue);
        xlsx.write(row,   1, "Выс. ветр. зонд., км",   fmtLabel);
        xlsx.write(row++, 2, m.reachedWindKm,          fmtValue);
        row++;
    };

    if (opts.includeMeteo11Updated)  writeM11(snap.meteo11Updated,     "Уточнённый");
    if (opts.includeMeteo11Approx)   writeM11(snap.meteo11Approximate, "Приближённый");
    if (opts.includeMeteo11Station)  writeM11(snap.meteo11FromStation, "От метеостанции");
}

if (!xlsx.saveAs(filePath)) {
    errorMsg = "Не удалось сохранить XLSX-файл: " + filePath;
    return false;
}
return true;


}

// ══════════════════════════════════════════════════════════════════════════════
// suggestedFileName()
// ══════════════════════════════════════════════════════════════════════════════

QString MeasurementExporter::suggestedFileName(const MeasurementSnapshot &snap,
                                               ExportOptions::Format      format)
{
    QString dt = snap.measurementTime.isValid()
                     ? snap.measurementTime.toString("yyyyMMdd_HHmmss")
                     : "unknown";
    return QString("measurement_%1_ID%2.%3")
        .arg(dt).arg(snap.recordId).arg(formatExt(format));
}

// ══════════════════════════════════════════════════════════════════════════════
// TXT
// ══════════════════════════════════════════════════════════════════════════════

QString MeasurementExporter::generateTxt(const MeasurementSnapshot &s,
                                         const ExportOptions       &o)
{
    const QString SEP(80, '=');
    const QString sub(80, '-');
    QStringList out;


    auto h1 = [&](const QString &t) { out << SEP << "  " + t << SEP; };
    auto h2 = [&](const QString &t) { out << "" << sub << "  " + t << sub; };
    auto kv = [&](const QString &k, const QString &v) {
        out << QString("  %1 %2").arg(k + ":", -46).arg(v);
    };

    h1("РЕЗУЛЬТАТЫ ИЗМЕРЕНИЙ — МЕТЕОКОМПЛЕКС");
    kv("Запись ID",  QString::number(s.recordId));
    kv("Дата/время", s.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    if (!s.stationNumber.isEmpty()) kv("Номер станции", s.stationNumber);

    if (o.includeCoordinates) {
        h2("КООРДИНАТЫ СТАНЦИИ");
        if (s.coordinatesValid) {
            kv("Широта",    latToStr(s.latitude));
            kv("Долгота",   lonToStr(s.longitude));
            kv("Высота, м", QString::number(s.altitude,'f',1));
        } else { out << "  — нет данных —"; }
    }

    if (o.includeSurfaceMeteo) {
        h2("НАЗЕМНЫЕ МЕТЕОРОЛОГИЧЕСКИЕ УСЛОВИЯ");
        if (s.surfaceMeteoValid) {
            kv("Давление (P), мм рт.ст.",  QString::number(s.pressureHpa,     'f',1));
            kv("Температура (T), °C",       QString::number(s.temperatureC,    'f',1));
            kv("Относит. влажность (r), %", QString::number(s.humidityPct,     'f',1));
            kv("Направление ветра (A), °",  QString::number(s.surfaceWindDir,  'f',0));
            kv("Скорость ветра (V), м/с",   QString::number(s.surfaceWindSpeed,'f',1));
        } else { out << "  — нет данных —"; }
    }

    auto windBlock = [&](const QString &title,
                         const QVector<WindProfileData> &data, bool inc) {
        if (!inc) return;
        h2(title);
        if (data.isEmpty()) { out << "  — нет данных —"; return; }
        out << "  Высота, м       Скорость, м/с   Направление, °";
        out << "  " + QString(14,'-') + "  " + QString(14,'-') + "  " + QString(14,'-');
        for (const WindProfileData &p : data)
            out << QString("  %1  %2  %3")
                       .arg(QString::number(p.height,'f',0),    -14)
                       .arg(QString::number(p.windSpeed,'f',2), -14)
                       .arg(QString::number(p.windDirection),   -14);
    };

    windBlock("СРЕДНИЙ ВЕТЕР",        s.avgWind,    o.includeAvgWind);
    windBlock("ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР", s.actualWind, o.includeActualWind);

    if (o.includeMeasuredWind) {
        h2("ИЗМЕРЕННЫЙ ВЕТЕР");
        if (s.measuredWind.isEmpty()) { out << "  — нет данных —"; }
        else {
            out << "  Высота, м       Скорость, м/с   Направление, °";
            out << "  " + QString(14,'-') + "  " + QString(14,'-') + "  " + QString(14,'-');
            for (const MeasuredWindData &p : s.measuredWind)
                out << QString("  %1  %2  %3")
                           .arg(QString::number(p.height,'f',0),    -14)
                           .arg(QString::number(p.windSpeed,'f',2), -14)
                           .arg(QString::number(p.windDirection),   -14);
        }
    }

    if (o.includeWindShear) {
        h2("СДВИГ ВЕТРА");
        if (s.windShear.isEmpty()) { out << "  — нет данных —"; }
        else {
        out << "  Высота, м    Скор. м/с/30м       Изм. нап-я, °        Уровень";
        for (const WindShearData &w : s.windShear)
            out << QString("  %1  %2  %3  %4")
                       .arg(QString::number(static_cast<int>(w.height)), -12)
                       .arg(QString::number(w.shearPer30m,'f',2),        -18)
                       .arg(QString::number(w.shearDirection,'f',1),     -20)
                       .arg(WindShearCalculator::getSeverityText(w.severityLevel));
        }
    }

    auto m11Block = [&](const QString &title,
                        const MeasurementSnapshot::Meteo11Export &m, bool inc) {
        if (!inc) return;
        h2(title);
        if (!m.valid) { out << "  — нет данных —"; return; }
        out << "  " + m.bulletinString << "";
        kv("Номер ст. (NNNNN)",    m.stationNumber);
        kv("BBBB (высота ст.)",    QString("%1").arg(m.stationAltitude,4,10,QChar('0')));
        kv("БББ (откл. давл.)",    QString("%1").arg(m.pressureDev,3,10,QChar('0')));
        kv("ТТ (откл. темп.)",     QString("%1").arg(m.tempVirtDev,2,10,QChar('0')));
        kv("Выс. темп. зонд., км", QString::number(m.reachedTempKm));
        kv("Выс. ветр. зонд., км", QString::number(m.reachedWindKm));
    };

    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (УТОЧНЁННЫЙ)",      s.meteo11Updated,     o.includeMeteo11Updated);
    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (ПРИБЛИЖЁННЫЙ)",    s.meteo11Approximate, o.includeMeteo11Approx);
    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (ОТ МЕТЕОСТАНЦИИ)", s.meteo11FromStation, o.includeMeteo11Station);

    out << "" << SEP
        << "  Сформировано: " +
               QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") +
               "  |  Метеокомплекс АМС"
        << SEP;

    return out.join('\n');

}

// ══════════════════════════════════════════════════════════════════════════════
// CSV
// ══════════════════════════════════════════════════════════════════════════════

static QString csvQ(QChar sep, const QStringList &cols)
{
    QStringList q;
    for (const QString &c : cols) {
        QString s = c;
        s.replace('"', """");
        if (s.contains(sep) || s.contains('\n') || s.contains('"'))
            s = """ + s + """;
        q << s;
    }
    return q.join(sep);
}

QString MeasurementExporter::generateCsv(const MeasurementSnapshot &s,
                                         const ExportOptions       &o)
{
    const QChar S = o.csvSeparator;
    QStringList out;


    out << csvQ(S, {"[ЗАПИСЬ]",""}); out << csvQ(S, {"Номер записи", QString::number(s.recordId)});
    out << csvQ(S, {"Дата/время", s.measurementTime.toString("dd.MM.yyyy hh:mm:ss")});
    out << csvQ(S, {"Станция",  s.stationNumber}); out << "";

    if (o.includeCoordinates) {
        out << csvQ(S, {"[КООРДИНАТЫ]",""});
        if (s.coordinatesValid) {
            out << csvQ(S, {"Широта",  latToStr(s.latitude)});
            out << csvQ(S, {"Долгота", lonToStr(s.longitude)});
            out << csvQ(S, {"Высота, м",QString::number(s.altitude,'f',1)});
        } else out << csvQ(S, {"","нет данных"});
        out << "";
    }
    if (o.includeSurfaceMeteo) {
        out << csvQ(S, {"[НАЗЕМНЫЕ МЕТ. УСЛОВИЯ]",""});
        if (s.surfaceMeteoValid) {
            out << csvQ(S, {"Давление, мм рт.ст.",QString::number(s.pressureHpa,    'f',1)});
            out << csvQ(S, {"Температура, C",QString::number(s.temperatureC,   'f',1)});
            out << csvQ(S, {"Влажность, %", QString::number(s.humidityPct,    'f',1)});
            out << csvQ(S, {"Напр. ветра, град.", QString::number(s.surfaceWindDir, 'f',0)});
            out << csvQ(S, {"Скор. ветра, м/с",QString::number(s.surfaceWindSpeed,'f',1)});
        } else out << csvQ(S, {"","нет данных"});
        out << "";
    }

    auto windSec = [&](const QString &tag, const QVector<WindProfileData> &data, bool inc) {
        if (!inc) return;
        out << csvQ(S,{tag,"",""});
        out << csvQ(S,{"Высота, м","Скорость, м/с","Направление, град"});
        if (!data.isEmpty())
            for (const WindProfileData &p : data)
                out << csvQ(S,{QString::number(p.height,'f',0),
                                QString::number(p.windSpeed,'f',2),
                                QString::number(p.windDirection)});
        else out << csvQ(S,{"Нет данных","",""});
        out << "";
    };
    windSec("[СРЕДНИЙ ВЕТЕР]",        s.avgWind,    o.includeAvgWind);
    windSec("[ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР]", s.actualWind, o.includeActualWind);

    if (o.includeMeasuredWind) {
        out << csvQ(S,{"[ИЗМЕРЕННЫЙ ВЕТЕР]","",""});
        out << csvQ(S,{"Высота, м","Скорость, м/с","Направление, град."});
        if (!s.measuredWind.isEmpty())
            for (const MeasuredWindData &p : s.measuredWind)
                out << csvQ(S,{QString::number(p.height,'f',0),
                                QString::number(p.windSpeed,'f',2),
                                QString::number(p.windDirection)});
        else out << csvQ(S,{"нет данных","",""});
        out << "";
    }
    if (o.includeWindShear) {
        out << csvQ(S,{"[СДВИГ ВЕТРА]","","",""});
        out << csvQ(S,{"Высота, м","Скор. сдвига, м/с/30м","Изм. направления, град.","Уровень"});
        if (!s.windShear.isEmpty())
            for (const WindShearData &w : s.windShear)
                out << csvQ(S,{QString::number(static_cast<int>(w.height)),
                                QString::number(w.shearPer30m,'f',2),
                                QString::number(w.shearDirection,'f',1),
                                WindShearCalculator::getSeverityText(w.severityLevel)});
        else out << csvQ(S,{"нет данных","","",""});
        out << "";
    }

    auto m11csv = [&](const QString &tag,
                      const MeasurementSnapshot::Meteo11Export &m, bool inc) {
        if (!inc) return;
        out << csvQ(S,{tag,""});
        if (m.valid) {
            out << csvQ(S,{"Строка бюллетеня",       m.bulletinString});
            out << csvQ(S,{"NNNNN (ном. ст.)",           m.stationNumber});
            out << csvQ(S,{"BBBB (высота ст.)",            QString::number(m.stationAltitude)});
            out << csvQ(S,{"БББ (откл. давл.)",             QString::number(m.pressureDev)});
            out << csvQ(S,{"TT (откл. темп.)",            QString::number(m.tempVirtDev)});
            out << csvQ(S,{"Выс. темп. зонд., км", QString::number(m.reachedTempKm)});
            out << csvQ(S,{"Выс. ветр. зонд., км", QString::number(m.reachedWindKm)});
        } else out << csvQ(S,{"","нет данных"});
        out << "";
    };
    m11csv("[МЕТЕО-11 УТОЧНЁННЫЙ]",      s.meteo11Updated,     o.includeMeteo11Updated);
    m11csv("[МЕТЕО-11 ПРИБЛИЖЁННЫЙ]",    s.meteo11Approximate, o.includeMeteo11Approx);
    m11csv("[МЕТЕО-11 ОТ МЕТЕОСТАНЦИИ]", s.meteo11FromStation, o.includeMeteo11Station);

    out << csvQ(S,{"[СФОРМИРОВАНО]",
                    QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")});
    return out.join('\n');


}

// ══════════════════════════════════════════════════════════════════════════════
// JSON
// ══════════════════════════════════════════════════════════════════════════════

QString MeasurementExporter::generateJson(const MeasurementSnapshot &s,
                                          const ExportOptions       &o)
{
    QJsonObject root;
    QJsonObject meta;
    meta["record_id"]=s.recordId; meta["datetime"]=s.measurementTime.toString(Qt::ISODate);
    meta["station_number"]=s.stationNumber;
    meta["exported_at"]=QDateTime::currentDateTime().toString(Qt::ISODate);
    root["meta"]=meta;


    if (o.includeCoordinates) {
        QJsonObject c; c["valid"]=s.coordinatesValid;
        if (s.coordinatesValid){c["latitude"]=s.latitude;c["longitude"]=s.longitude;c["altitude_m"]=s.altitude;}
        root["coordinates"]=c;
    }
    if (o.includeSurfaceMeteo) {
        QJsonObject sm; sm["valid"]=s.surfaceMeteoValid;
        if (s.surfaceMeteoValid){sm["pressure_hpa"]=s.pressureHpa;sm["temperature_c"]=s.temperatureC;
            sm["humidity_pct"]=s.humidityPct;sm["wind_dir_deg"]=s.surfaceWindDir;sm["wind_speed_ms"]=s.surfaceWindSpeed;}
        root["surface_meteo"]=sm;
    }
    auto windArr=[](const QVector<WindProfileData>&data){
        QJsonArray arr;
        for(const WindProfileData&p:data){QJsonObject o;o["h"]=p.height;o["spd"]=p.windSpeed;o["dir"]=p.windDirection;arr.append(o);}
        return arr;};
    if (o.includeAvgWind){QJsonObject w;w["count"]=s.avgWind.size();w["data"]=windArr(s.avgWind);root["avg_wind"]=w;}
    if (o.includeActualWind){QJsonObject w;w["count"]=s.actualWind.size();w["data"]=windArr(s.actualWind);root["actual_wind"]=w;}
    if (o.includeMeasuredWind){
        QJsonArray arr;
        for(const MeasuredWindData&p:s.measuredWind){QJsonObject o;o["h"]=p.height;o["spd"]=p.windSpeed;o["dir"]=p.windDirection;o["rel"]=p.reliability;arr.append(o);}
        QJsonObject w;w["count"]=s.measuredWind.size();w["data"]=arr;root["measured_wind"]=w;
    }
    if (o.includeWindShear){
        QJsonArray arr;
        for(const WindShearData&w:s.windShear){QJsonObject o;o["h"]=w.height;o["shear"]=w.shearPer30m;o["dir_chg"]=w.shearDirection;o["severity"]=w.severityLevel;o["sev_text"]=WindShearCalculator::getSeverityText(w.severityLevel);arr.append(o);}
        QJsonObject sh;sh["count"]=s.windShear.size();sh["data"]=arr;root["wind_shear"]=sh;
    }
    auto m11js=[](const MeasurementSnapshot::Meteo11Export&m){
        QJsonObject o;o["valid"]=m.valid;
        if(m.valid){o["bulletin"]=m.bulletinString;o["station_number"]=m.stationNumber;o["BBBB"]=m.stationAltitude;o["BBB_pressure_dev"]=m.pressureDev;o["T0T0_temp_dev"]=m.tempVirtDev;o["reached_temp_km"]=m.reachedTempKm;o["reached_wind_km"]=m.reachedWindKm;}
        return o;};
    if(o.includeMeteo11Updated)  root["meteo11_updated"]     =m11js(s.meteo11Updated);
    if(o.includeMeteo11Approx)   root["meteo11_approximate"] =m11js(s.meteo11Approximate);
    if(o.includeMeteo11Station)  root["meteo11_from_station"]=m11js(s.meteo11FromStation);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);


}

// ══════════════════════════════════════════════════════════════════════════════
// PDF HTML helpers
// ══════════════════════════════════════════════════════════════════════════════

static const char *kCss =
    "body{font-family:Arial,sans-serif;font-size:9pt;color:#111;margin:0;padding:0}"
    "h1{font-size:13pt;color:#1565C0;margin:0 0 4px}"
    ".hdr{background:#1565C0;color:#fff;padding:10px 14px;margin-bottom:14px}"
    ".hdr .sub{font-size:8pt;opacity:.85;margin-top:3px}"
    "h2{font-size:10pt;color:#1565C0;border-bottom:1.5px solid #1565C0;padding-bottom:2px;margin:14px 0 6px}"
    "table{border-collapse:collapse;width:100%;font-size:8.5pt}"
    "th{background:#1976D2;color:#fff;padding:4px 8px;text-align:center;border:1px solid #1565C0}"
    "td{padding:3px 8px;text-align:center;border:1px solid #ccc}"
    "tr:nth-child(even) td{background:#EEF4FB}"
    ".kv{width:100%;border-collapse:collapse;font-size:9pt;margin-bottom:6px}"
    ".kv td{border:none;padding:2px 6px}"
    ".kv .k{font-weight:bold;width:46%;color:#333}"
    ".bulletin{font-family:'Courier New',monospace;font-size:9.5pt;background:#F5F5F5;"
    "padding:8px 10px;border-left:3px solid #1976D2;word-break:break-all;margin-bottom:6px}"
    ".nodata{color:#999;font-style:italic}"
    ".chart{width:100%;margin:6px 0 10px}"
    ".sev1{background:#C8E6C9}.sev2{background:#FFF9C4}"
    ".sev3{background:#FFCCBC}.sev4{background:#FFCDD2}";

static QString chartImg(const QMap<QString,QImage> &charts, const QString &key)
{
    if (!charts.contains(key) || charts[key].isNull()) return {};
    const QImage &img = charts[key];
    return QString("<img src=\"img://%1\" width=\"%2\" height=\"%3\"/>").arg(key).arg(img.width()).arg(img.height());
}

QString MeasurementExporter::buildHtmlReport(const MeasurementSnapshot &s,
                                             const ExportOptions       &o)
{
    QString html;
    html.reserve(65536);
    html += "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"/>"
            "<style>" + QString(kCss) + "</style></head><body>";
    html += "<div class=\"hdr\"><h1>Результаты измерений — Метеокомплекс</h1>"
            "<div class=\"sub\">Запись ID: <b>" + QString::number(s.recordId) +
            "</b> | Дата/время: <b>" +
s.measurementTime.toString("dd.MM.yyyy hh:mm:ss") + "</b>";
            if (!s.stationNumber.isEmpty())
            html += " | Ст.: <b>" + s.stationNumber + "</b>";
    html += "</div></div>";


        if ((o.includeCoordinates&&s.coordinatesValid)||(o.includeSurfaceMeteo&&s.surfaceMeteoValid)) {
        html += "<table style=\"width:100%;border-collapse:collapse\"><tr>";
        if (o.includeCoordinates&&s.coordinatesValid)
            html += "<td style=\"vertical-align:top;width:50%;padding-right:10px\">"
                    "<h2>Координаты станции</h2><table class=\"kv\">"
                    "<tr><td class=\"k\">Широта</td><td class=\"v\">"  +latToStr(s.latitude)+"</td></tr>"
                                             "<tr><td class=\"k\">Долгота</td><td class=\"v\">" +lonToStr(s.longitude)+"</td></tr>"
                                              "<tr><td class=\"k\">Высота, м</td><td class=\"v\">"+QString::number(s.altitude,'f',1)+"</td></tr>"
                                                            "</table></td>";
        if (o.includeSurfaceMeteo&&s.surfaceMeteoValid)
            html += "<td style=\"vertical-align:top\"><h2>Наземные метеоусловия</h2>"
                    "<table class=\"kv\">"
                    "<tr><td class=\"k\">Давление, мм рт.ст.</td><td class=\"v\">"+QString::number(s.pressureHpa,'f',1)+"</td></tr>"
                                                               "<tr><td class=\"k\">Температура, °C</td><td class=\"v\">"+QString::number(s.temperatureC,'f',1)+"</td></tr>"
                                                                "<tr><td class=\"k\">Влажность, %</td><td class=\"v\">"+QString::number(s.humidityPct,'f',1)+"</td></tr>"
                                                               "<tr><td class=\"k\">Направление ветра, °</td><td class=\"v\">"+QString::number(s.surfaceWindDir,'f',0)+"</td></tr>"
                                                                  "<tr><td class=\"k\">Скорость ветра, м/с</td><td class=\"v\">"+QString::number(s.surfaceWindSpeed,'f',1)+"</td></tr>"
                                                                    "</table></td>";
        html += "</tr></table>";
    }

    auto windSec = [&](const QString &title, const QVector<WindProfileData>&data,
                       bool inc, const QString &keySpd, const QString &keyDir){
        if (!inc) return;
        html += "<h2>" + title + "</h2>";
        if (data.isEmpty()){html+="<p class=\"nodata\">— нет данных —</p>";return;}
        if (o.pdfCharts)
            html += "<table style=\"width:100%\"><tr>"
                    "<td>"+chartImg(s.charts,keySpd)+"</td>"
                                                   "<td>"+chartImg(s.charts,keyDir)+"</td></tr></table>";
        html += htmlWindTable(data);
    };
    windSec("Средний ветер",        s.avgWind,    o.includeAvgWind,    "avgSpeed",    "avgDir");
    windSec("Действительный ветер", s.actualWind, o.includeActualWind, "actualSpeed", "actualDir");

    if (o.includeMeasuredWind) {
        html += "<h2>Измеренный ветер</h2>";
        if (!s.measuredWind.isEmpty()) {
            if (o.pdfCharts)
                html += "<table style=\"width:100%\"><tr>"
                        "<td>"+chartImg(s.charts,"measSpeed")+"</td>"
                                                            "<td>"+chartImg(s.charts,"measDir")+"</td></tr></table>";
            html += htmlMeasTable(s.measuredWind);
        } else html += "<p class=\"nodata\">— нет данных —</p>";
    }
    if (o.includeWindShear) {
        html += "<h2>Сдвиг ветра</h2>";
        if (!s.windShear.isEmpty()) {
            if (o.pdfCharts)
                html += "<table style=\"width:100%\"><tr>"
                        "<td>"+chartImg(s.charts,"shearSpeed")+"</td>"
                                                             "<td>"+chartImg(s.charts,"shearDir")+"</td></tr></table>";
            html += htmlShearTable(s.windShear);
        } else html += "<p class=\"nodata\">— нет данных —</p>";
    }
    if (o.includeMeteo11Updated)  html += htmlMeteo11Block(s.meteo11Updated,     "Бюллетень Метео-11 (уточнённый)");
    if (o.includeMeteo11Approx)   html += htmlMeteo11Block(s.meteo11Approximate, "Бюллетень Метео-11 (приближённый)");
    if (o.includeMeteo11Station)  html += htmlMeteo11Block(s.meteo11FromStation, "Бюллетень Метео-11 (от метеостанции)");

    html += "<hr style=\"border:none;border-top:1px solid #ccc;margin-top:14px\"/>"
            "<p style=\"font-size:7.5pt;color:#888;text-align:right\">Сформировано: "+
            QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")+
            " &nbsp;|&nbsp; Метеокомплекс АМС</p></body></html>";
    return html;


}

QString MeasurementExporter::htmlWindTable(const QVector<WindProfileData> &data)
{
    QString t="<table><tr><th>Высота, м</th><th>Скорость, м/с</th><th>Направление, °</th></tr>";
    for (const WindProfileData &p:data)
        t+=QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                 .arg(static_cast<int>(p.height)).arg(p.windSpeed,0,'f',2).arg(p.windDirection);
    return t+"</table>";
}

QString MeasurementExporter::htmlMeasTable(const QVector<MeasuredWindData> &data)
{
    QString t="<table><tr><th>Высота, м</th><th>Скорость, м/с</th><th>Направление, °</th></tr>";
    for (const MeasuredWindData &p:data)
        t+=QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                 .arg(static_cast<int>(p.height)).arg(p.windSpeed,0,'f',2).arg(p.windDirection);
    return t+"</table>";
}

QString MeasurementExporter::htmlShearTable(const QVector<WindShearData> &data)
{
    QString t="<table><tr><th>Высота, м</th><th>Скорость, м/с/30м</th>"
                "<th>Изм. направления, °</th><th>Уровень</th></tr>";
    for (const WindShearData &w:data){
        QString cls;
        if      (w.severityLevel==1) cls=" class=\"sev1\"";
        else if (w.severityLevel==2) cls=" class=\"sev2\"";
        else if (w.severityLevel==3) cls=" class=\"sev3\"";
        else if (w.severityLevel>=4) cls=" class=\"sev4\"";
        t+=QString("<tr%1><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                 .arg(cls).arg(static_cast<int>(w.height))
                 .arg(w.shearPer30m,0,'f',2).arg(w.shearDirection,0,'f',1)
                 .arg(WindShearCalculator::getSeverityText(w.severityLevel).toHtmlEscaped());
    }
    return t+"</table>";
}

QString MeasurementExporter::htmlMeteo11Block(
    const MeasurementSnapshot::Meteo11Export &m, const QString &title)
{
    QString html="<h2>"+title.toHtmlEscaped()+"</h2>";
    if (!m.valid) return html+"<p class=\"nodata\">— нет данных —</p>";
    html+="<div class=\"bulletin\">"+m.bulletinString.toHtmlEscaped()+"</div>"
                                                                            "<table class=\"kv\">"
                                                                            "<tr><td class=\"k\">Номер ст. (NNNNN)</td><td class=\"v\">"+m.stationNumber+"</td></tr>"
                                "<tr><td class=\"k\">BBBB (высота ст.)</td><td class=\"v\">"+QString("%1").arg(m.stationAltitude,4,10,QChar('0'))+"</td></tr>"
                                                                        "<tr><td class=\"k\">БББ (откл. давл.)</td><td class=\"v\">"+QString("%1").arg(m.pressureDev,3,10,QChar('0'))+"</td></tr>"
                                                                    "<tr><td class=\"k\">ТТ (откл. темп.)</td><td class=\"v\">"+QString("%1").arg(m.tempVirtDev,2,10,QChar('0'))+"</td></tr>"
                                                                    "<tr><td class=\"k\">Выс. темп. зонд., км</td><td class=\"v\">"+QString::number(m.reachedTempKm)+"</td></tr>"
                                                 "<tr><td class=\"k\">Выс. ветр. зонд., км</td><td class=\"v\">"+QString::number(m.reachedWindKm)+"</td></tr>"
                                                 "</table>";
    return html;
}

// ══════════════════════════════════════════════════════════════════════════════
// Утилиты
// ══════════════════════════════════════════════════════════════════════════════

QString MeasurementExporter::latToStr(double lat)
{
    double a=qAbs(lat); int d=static_cast<int>(a);
    double mf=(a-d)*60.0; int m=static_cast<int>(mf); double sc=(mf-m)*60.0;
    return QString("%1°%2'%3\" %4").arg(d).arg(m,2,10,QChar('0'))
        .arg(sc,5,'f',2,QChar('0')).arg(lat>=0?"С":"Ю");
}

QString MeasurementExporter::lonToStr(double lon)
{
    double a=qAbs(lon); int d=static_cast<int>(a);
    double mf=(a-d)*60.0; int m=static_cast<int>(mf); double sc=(mf-m)*60.0;
    return QString("%1°%2'%3\" %4").arg(d).arg(m,2,10,QChar('0'))
        .arg(sc,5,'f',2,QChar('0')).arg(lon>=0?"В":"З");
}

QString MeasurementExporter::formatExt(ExportOptions::Format fmt)
{
    switch(fmt){
    case ExportOptions::TXT:  return "txt";
    case ExportOptions::CSV:  return "csv";
    case ExportOptions::JSON: return "json";
    case ExportOptions::PDF:  return "pdf";
    case ExportOptions::XLSX: return "xlsx";
    }
    return "txt";
}