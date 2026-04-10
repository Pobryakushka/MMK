#include "MeasurementExporter.h"
#include "amsprotocol.h"        // WindProfileData, MeasuredWindData
#include "WindShearCalculator.h" // WindShearData
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
#include "xlsxcellrange.h"

// ─────────────────────────────────────────────────────────────────────────────
// Интерфейс
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

QString MeasurementExporter::generate(const MeasurementSnapshot &snap,
                                      const ExportOptions       &opts,
                                      QString                   &errorMsg)
{
    errorMsg.clear();

    // Базовая проверка: есть ли хоть что-то для экспорта?
    if (snap.recordId <= 0) {
        errorMsg = "Нет загруженных данных для экспорта.\n"
                   "Выберите дату и время с доступными измерениями.";
        return {};
    }

    if (!hasAnyData(snap)) {
        errorMsg = QString("Запись ID %1 от %2 не содержит данных ни в одном разделе.")
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

bool MeasurementExporter::generatePdf(const MeasurementSnapshot &snap, const ExportOptions &opts,
                                      const QString &filePath, QString &errorMsg)
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
    printer.setPageOrientation(opts.pdfLandscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setDefaultStyleSheet("body { font-family: Arial, sans-serif; font-size: 9pt; color: #111; }"
                             );

    if (opts.pdfCharts) {
        for (auto it = snap.charts.constBegin(); it != snap.charts.constEnd(); ++it) {
            if (!it.value().isNull())
                doc.addResource(QTextDocument::ImageResource,
                                QUrl("img://" + it.key()),
                                QVariant(it.value()));
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
                                       const ExportOptions &opts, const QString &filePath, QString &errorMsg)
{
    errorMsg.clear();

    if (snap.recordId <= 0 || !hasAnyData(snap)) {
        errorMsg = "Нет данных для формирования XLSX.";
        return false;
    }

    QXlsx::Document xlsx;

    QXlsx::Format fmtTitle;
    fmtTitle.setFontSize(13);
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

    // --------- ЛИСТ 1 - СВОДКА ------------
    xlsx.renameSheet("Sheet1", "Сводка");
    xlsx.setColumnWidth(1, 40);
    xlsx.setColumnWidth(2, 30);

    auto write2 = [&](int row, const QString &label, const QString &value) {
        xlsx.write(row, 1, label, fmtLabel);
        xlsx.write(row, 2, value, fmtValue);
    };

    xlsx.write(1, 1, "Результаты измерений - Метеокомплекс", fmtTitle);
    int r = 3;
    write2(r++, "Запись ID", QString::number(snap.recordId));
    write2(r++, "Дата/время", snap.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    write2(r++, "Ном. станции", snap.stationNumber);
    r++;

    if (opts.includeCoordinates && snap.coordinatesValid) {
        xlsx.write(r++, 1, "Кооридинаты станции", fmtTitle);
        write2(r++, "Широта", latToStr(snap.latitude));
        write2(r++, "Долгота", lonToStr(snap.longitude));
        write2(r++, "Высота над УМ, м", QString::number(snap.altitude, 'f', 1));
        r++;
    }

    if (opts.includeSurfaceMeteo && snap.surfaceMeteoValid) {
        xlsx.write(r++, 1, "Наземные метеоусловия", fmtTitle);
        write2(r++, "Давление, мм рт.ст.", QString::number(snap.pressureMmHg, 'f', 1));
        write2(r++, "Температура, °C", QString::number(snap.temperatureC, 'f', 1));
        write2(r++, "Влажность, %", QString::number(snap.humidityPct, 'f', 1));
        write2(r++, "Направление ветра, °", QString::number(snap.surfaceWindDir, 'f', 0));
        write2(r++, "Скорость ветра, м/с", QString::number(snap.surfaceWindSpeed, 'f', 1));
        r++;
    }

    // ---------- ЛИСТ 2 - СРЕДНИЙ ВЕТЕР ------------
    if (opts.includeAvgWind && !snap.avgWind.isEmpty()) {
        xlsx.addSheet("Средний ветер");
        xlsx.setColumnWidth(1, 14);
        xlsx.setColumnWidth(2, 16);
        xlsx.setColumnWidth(3, 18);

        xlsx.write(1, 1, "Средний ветер", fmtTitle);
        xlsx.write(3, 1, "Высота, м", fmtHeader);
        xlsx.write(3, 2, "Скорость, м/с", fmtHeader);
        xlsx.write(3, 3, "Направление, °", fmtHeader);
        int row = 4;
        for (const WindProfileData &p : snap.avgWind) {
            QXlsx::Format &fmt = (row % 2 == 0) ? fmtCellAlt : fmtCell;
            xlsx.write(row, 1, p.height, fmt);
            xlsx.write(row, 2, p.windSpeed, fmt);
            xlsx.write(row, 3, p.windDirection, fmt);
            ++row;
        }
    }

    // ------------ ЛИСТ 3 - ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР ------------------
    if (opts.includeActualWind && !snap.actualWind.isEmpty()) {
        xlsx.addSheet("Действ. ветер");
        xlsx.setColumnWidth(1, 14);
        xlsx.setColumnWidth(2, 16);
        xlsx.setColumnWidth(3, 18);

        xlsx.write(1, 1, "Действительный ветер", fmtTitle);
        xlsx.write(3, 1, "Высота, м",      fmtHeader);
        xlsx.write(3, 2, "Скорость, м/с",  fmtHeader);
        xlsx.write(3, 3, "Направление, °", fmtHeader);
        int row = 4;
        for (const WindProfileData &p : snap.actualWind) {
            QXlsx::Format &fmt = (row % 2 == 0) ? fmtCellAlt : fmtCell;
            xlsx.write(row, 1, p.height,       fmt);
            xlsx.write(row, 2, p.windSpeed,     fmt);
            xlsx.write(row, 3, p.windDirection, fmt);
            ++row;
        }
    }

    // -------------- ЛИСТ 4 - ИЗМЕРЕННЫЙ ВЕТЕР ---------------
    if (opts.includeMeasuredWind && !snap.measuredWind.isEmpty()) {
        xlsx.addSheet("Измер. ветер");
        xlsx.setColumnWidth(1, 14);
        xlsx.setColumnWidth(2, 16);
        xlsx.setColumnWidth(3, 18);

        xlsx.write(1, 1, "Измеренный ветер", fmtTitle);
        xlsx.write(3, 1, "Высота, м",      fmtHeader);
        xlsx.write(3, 2, "Скорость, м/с",  fmtHeader);
        xlsx.write(3, 3, "Направление, °", fmtHeader);
        int row = 4;
        for (const MeasuredWindData &p : snap.measuredWind) {
            QXlsx::Format &fmt = (row % 2 == 0) ? fmtCellAlt : fmtCell;
            xlsx.write(row, 1, p.height,       fmt);
            xlsx.write(row, 2, p.windSpeed,     fmt);
            xlsx.write(row, 3, p.windDirection, fmt);
            ++row;
        }
    }

    // ------------- ЛИСТ 5 - Сдвиг ветра --------------------
    if (opts.includeWindShear && !snap.windShear.isEmpty()) {
        xlsx.addSheet("Сдвиг ветра");
        xlsx.setColumnWidth(1, 14);
        xlsx.setColumnWidth(2, 22);
        xlsx.setColumnWidth(3, 26);
        xlsx.setColumnWidth(4, 14);

        xlsx.write(1, 1, "Сдвиг ветра", fmtTitle);
        xlsx.write(3, 1, "Высота, м",               fmtHeader);
        xlsx.write(3, 2, "Скорость, м/с/30м",        fmtHeader);
        xlsx.write(3, 3, "Изм. направления, °",      fmtHeader);
        xlsx.write(3, 4, "Уровень",                  fmtHeader);

        // Цвета для уровней сдвига
        QXlsx::Format fmtLow, fmtMed, fmtHigh, fmtCrit;
        fmtLow.setPatternBackgroundColor(QColor("#C8E6C9"));
        fmtMed.setPatternBackgroundColor(QColor("#FFF9C4"));
        fmtHigh.setPatternBackgroundColor(QColor("#FFCCBC"));
        fmtCrit.setPatternBackgroundColor(QColor("#FFCDD2"));
        for (auto *f : {&fmtLow, &fmtMed, &fmtHigh, &fmtCrit}) {
            f->setHorizontalAlignment(QXlsx::Format::AlignHCenter);
            f->setBorderStyle(QXlsx::Format::BorderThin);
        }

        int row = 4;
        for (const WindShearData &s : snap.windShear) {
            QXlsx::Format *sf = &fmtCell;
            if      (s.severityLevel == 0) sf = &fmtLow;
            else if (s.severityLevel == 1) sf = &fmtMed;
            else if (s.severityLevel == 2) sf = &fmtHigh;
            else if (s.severityLevel >= 3) sf = &fmtCrit;

            xlsx.write(row, 1, static_cast<int>(s.height), *sf);
            xlsx.write(row, 2, s.shearPer30m,               *sf);
            xlsx.write(row, 3, s.shearDirection,             *sf);
            xlsx.write(row, 4,
                       WindShearCalculator::getSeverityText(s.severityLevel), *sf);
            ++row;
        }
    }

    // -------------- ЛИСТ 6 - МЕТЕО-11 --------------------
    if (opts.includeMeteo11Updated || opts.includeMeteo11Approx || opts.includeMeteo11Station) {
        xlsx.addSheet("Метео-11");
        xlsx.setColumnWidth(1, 36);
        xlsx.setColumnWidth(2, 50);

        int row = 1;
        auto writeM11 = [&](const MeasurementSnapshot::Meteo11Export &m,
                            const QString &label) {
            if (!m.valid) return;
            xlsx.write(row++, 1, label, fmtTitle);
            xlsx.write(row,   1, "Бюллетень",          fmtLabel);
            xlsx.write(row++, 2, m.bulletinString,      fmtValue);
            xlsx.write(row,   1, "Номер ст. (NNNNN)",   fmtLabel);
            xlsx.write(row++, 2, m.stationNumber,       fmtValue);
            xlsx.write(row,   1, "Высота ст. (BBBB)",   fmtLabel);
            xlsx.write(row++, 2, m.stationAltitude,     fmtValue);
            xlsx.write(row,   1, "Откл. давл. (БББ)",   fmtLabel);
            xlsx.write(row++, 2, m.pressureDev,         fmtValue);
            xlsx.write(row,   1, "Откл. т-ры (ТТ)",    fmtLabel);
            xlsx.write(row++, 2, m.tempVirtDev,         fmtValue);
            xlsx.write(row,   1, "Выс. темп. зонд., км",fmtLabel);
            xlsx.write(row++, 2, m.reachedTempKm,       fmtValue);
            xlsx.write(row,   1, "Выс. ветр. зонд., км",fmtLabel);
            xlsx.write(row++, 2, m.reachedWindKm,       fmtValue);
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

QString MeasurementExporter::suggestedFileName(const MeasurementSnapshot &snap,
                                               ExportOptions::Format       format)
{
    QString dt = snap.measurementTime.isValid()
    ? snap.measurementTime.toString("yyyyMMdd_HHmmss")
    : "unknown";
    return QString("measurement_%1_ID%2.%3")
        .arg(dt)
        .arg(snap.recordId)
        .arg(formatExt(format));
}

// ─────────────────────────────────────────────────────────────────────────────
// TXT
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::generateTxt(const MeasurementSnapshot &s,
                                         const ExportOptions       &o)
{
    const QString SEP(80, '=');
    const QString sub(80, '-');
    QStringList out;

    auto h1 = [&](const QString &t) {out << SEP << "  " + t << SEP; };
    auto h2 = [&](const QString &t) { out << "" << sub << "  " + t << sub; };
    auto kv = [&](const QString &k, const QString &v) {
        out << QString("  %1 %2").arg(k + ":", -46).arg(v);
    };

    // ── Шапка ──────────────────────────────────────────────────────────────
    h1("РЕЗУЛЬТАТЫ ИЗМЕРЕНИЙ — МЕТЕОКОМПЛЕКС");
    kv("Запись ID", QString::number(s.recordId));
    kv("Дата/время", s.measurementTime.toString("dd.MM.yyyy hh:mm:ss"));
    if (!s.stationNumber.isEmpty()) kv("Номер станции", s.stationNumber);

    // ── Координаты ─────────────────────────────────────────────────────────
    if (o.includeCoordinates) {
        h2("КООРДИНАТЫ СТАНЦИИ");
        if (s.coordinatesValid) {
            kv("Широта",         latToStr(s.latitude));
            kv("Долгота",        lonToStr(s.longitude));
            kv("Высота, м",      QString::number(s.altitude, 'f', 1));
        } else { out << "  — нет данных —"; }
    }

    // ── Наземные метеоусловия ───────────────────────────────────────────────
    if (o.includeSurfaceMeteo) {
        h2("НАЗЕМНЫЕ МЕТЕОРОЛОГИЧЕСКИЕ УСЛОВИЯ");
        if (s.surfaceMeteoValid) {
            kv("Давление (P), мм рт.ст.",       QString::number(s.pressureMmHg,     'f', 1));
            kv("Температура (T), °C",            QString::number(s.temperatureC,    'f', 1));
            kv("Относит. влажность (r), %",      QString::number(s.humidityPct,     'f', 1));
            kv("Направление ветра (A), °",       QString::number(s.surfaceWindDir,  'f', 0));
            kv("Скорость ветра (V), м/с",        QString::number(s.surfaceWindSpeed,'f', 1));
        } else { out << "  — нет данных —"; }
    }

    auto windBlock = [&](const QString &title, const QVector<WindProfileData> &data, bool include) {
        if (!include) return;
        h2(title);
        if (data.isEmpty()) { out << "  — нет данных —"; return; }
        out << QString("  %-12s  %-14s  %s").arg("Высота, м","Скорость, м/с","Направление, °");
        out << QString("  %1  %2  %3").arg(QString(12,'-'),QString(14,'-'),QString(16,'-'));
        for (const WindProfileData &p : data)
            out << QString("  %-12s  %-14s  %s")
                       .arg(QString::number(p.height,'f',0), 12)
                       .arg(QString::number(p.windSpeed,'f',2), 14)
                       .arg(QString::number(p.windDirection), 16);
    };

    windBlock("СРЕДНИЙ ВЕТЕР",        s.avgWind,    o.includeAvgWind);
    windBlock("ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР", s.actualWind, o.includeActualWind);

    // ── Измеренный ветер ────────────────────────────────────────────────────
    if (o.includeMeasuredWind) {
        h2("ИЗМЕРЕННЫЙ ВЕТЕР");
        if (s.measuredWind.isEmpty()) { out << "  — нет данных —"; }
        else {
            out << QString("  %-12s  %-14s  %s").arg("Высота, м","Скорость, м/с","Направление, °");
            out << QString("  %1  %2  %3").arg(QString(12,'-'),QString(14,'-'),QString(16,'-'));
            for (const MeasuredWindData &p : s.measuredWind)
                out << QString("  %-12s  %-14s  %s")
                           .arg(QString::number(p.height,'f',0), 12)
                           .arg(QString::number(p.windSpeed,'f',2), 14)
                           .arg(QString::number(p.windDirection), 16);
        }
    }

    // ── Сдвиг ветра ─────────────────────────────────────────────────────────
    if (o.includeWindShear) {
        h2("СДВИГ ВЕТРА");
        if (s.windShear.isEmpty()) { out << "  — нет данных —"; }
        else {
            out << QString("  %-12s  %-18s  %-22s  %s")
            .arg("Высота, м","Скорость, м/с/30м","Изм. нап-я, °","Уровень");
            for (const WindShearData &w : s.windShear)
                out << QString("  %-12s  %-18s  %-22s  %s")
                           .arg(QString::number(static_cast<int>(w.height)), 12)
                           .arg(QString::number(w.shearPer30m,'f',2), 18)
                           .arg(QString::number(w.shearDirection,'f',1), 22)
                           .arg(WindShearCalculator::getSeverityText(w.severityLevel));
        }
    }

    // ── Метео-11 ────────────────────────────────────────────────────────────
    auto m11Block = [&](const QString &title,
                        const MeasurementSnapshot::Meteo11Export &m, bool include) {
        if (!include) return;
        h2(title);
        if (!m.valid) { out << "  — нет данных —"; return; }
        out << "  " + m.bulletinString;
        out << "";
        kv("Номер ст. (NNNNN)", m.stationNumber);
        kv("BBBB (высота ст.)", QString("%1").arg(m.stationAltitude, 4, 10, QChar('0')));
        kv("БББ (откл. давл.)", QString("%1").arg(m.pressureDev,     3, 10, QChar('0')));
        kv("ТТ (откл. темп.)",  QString("%1").arg(m.tempVirtDev,     2, 10, QChar('0')));
        kv("Выс. темп. зонд., км", QString::number(m.reachedTempKm));
        kv("Выс. ветр. зонд., км", QString::number(m.reachedWindKm));
    };

    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (УТОЧНЁННЫЙ)",       s.meteo11Updated,     o.includeMeteo11Updated);
    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (ПРИБЛИЖЁННЫЙ)",     s.meteo11Approximate, o.includeMeteo11Approx);
    m11Block("БЮЛЛЕТЕНЬ МЕТЕО-11 (ОТ МЕТЕОСТАНЦИИ)",  s.meteo11FromStation, o.includeMeteo11Station);

    out << "" << SEP
        << "  Сформировано: " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")
               + "  |  Метеокомплекс АМС"
        << SEP;

    return out.join('\n');

}

// ─────────────────────────────────────────────────────────────────────────────
// CSV
// ─────────────────────────────────────────────────────────────────────────────

static QString csvQ(QChar sep, const QStringList &cols)
{
    QStringList q;
    for (const QString &c : cols) {
        QString s = c;
        s.replace('"', "\"\"");
        if (s.contains(sep) || s.contains('\n') || s.contains('"'))
            s = "\"" + s + "\"";
        q << s;
    }
    return q.join(sep);
}

QString MeasurementExporter::generateCsv(const MeasurementSnapshot &s,
                                         const ExportOptions       &o)
{
    const QChar S = o.csvSeparator;
    QStringList out;

    out << csvQ(S, {"[ЗАПИСЬ]", ""});
    out << csvQ(S, {"record_id", QString::number(s.recordId)});
    out << csvQ(S, {"datetime",  s.measurementTime.toString("dd.MM.yyyy hh:mm:ss")});
    out << csvQ(S, {"station",   s.stationNumber});
    out << "";

    if (o.includeCoordinates) {
        out << csvQ(S, {"[КООРДИНАТЫ]", ""});
        if (s.coordinatesValid) {
            out << csvQ(S, {"latitude",   latToStr(s.latitude)});
            out << csvQ(S, {"longitude",  lonToStr(s.longitude)});
            out << csvQ(S, {"altitude_m", QString::number(s.altitude,'f',1)});
        } else { out << csvQ(S, {"","нет данных"}); }
        out << "";
    }

    if (o.includeSurfaceMeteo) {
        out << csvQ(S, {"[НАЗЕМНЫЕ МЕТ. УСЛОВИЯ]", ""});
        if (s.surfaceMeteoValid) {
            out << csvQ(S, {"pressure_mmhg",   QString::number(s.pressureMmHg,     'f',1)});
            out << csvQ(S, {"temperature_c",   QString::number(s.temperatureC,    'f',1)});
            out << csvQ(S, {"humidity_pct",    QString::number(s.humidityPct,     'f',1)});
            out << csvQ(S, {"wind_dir_deg",    QString::number(s.surfaceWindDir,  'f',0)});
            out << csvQ(S, {"wind_speed_ms",   QString::number(s.surfaceWindSpeed,'f',1)});
        } else { out << csvQ(S, {"","нет данных"}); }
        out << "";
    }

    auto windSection = [&](const QString &tag, const QVector<WindProfileData> &data, bool incl) {
        if (!incl) return;
        out << csvQ(S, {tag, "", ""});
        out << csvQ(S, {"height_m", "speed_ms", "direction_deg"});
        if (!data.isEmpty())
            for (const WindProfileData &p : data)
                out << csvQ(S, {QString::number(p.height,'f',0),
                                QString::number(p.windSpeed,'f',2),
                                QString::number(p.windDirection)});
        else out << csvQ(S, {"нет данных","",""});
        out << "";
    };
    windSection("[СРЕДНИЙ ВЕТЕР]",        s.avgWind,    o.includeAvgWind);
    windSection("[ДЕЙСТВИТЕЛЬНЫЙ ВЕТЕР]", s.actualWind, o.includeActualWind);

    if (o.includeMeasuredWind) {
        out << csvQ(S, {"[ИЗМЕРЕННЫЙ ВЕТЕР]","",""});
        out << csvQ(S, {"height_m","speed_ms","direction_deg"});
        if (!s.measuredWind.isEmpty())
            for (const MeasuredWindData &p : s.measuredWind)
                out << csvQ(S, {QString::number(p.height,'f',0),
                                QString::number(p.windSpeed,'f',2),
                                QString::number(p.windDirection)});
        else out << csvQ(S, {"нет данных","",""});
        out << "";
    }

    if (o.includeWindShear) {
        out << csvQ(S, {"[СДВИГ ВЕТРА]","","",""});
        out << csvQ(S, {"height_m","shear_ms_30m","dir_change_deg","severity"});
        if (!s.windShear.isEmpty())
            for (const WindShearData &w : s.windShear)
                out << csvQ(S, {QString::number(static_cast<int>(w.height)),
                                QString::number(w.shearPer30m,'f',2),
                                QString::number(w.shearDirection,'f',1),
                                WindShearCalculator::getSeverityText(w.severityLevel)});
        else out << csvQ(S, {"нет данных","","",""});
        out << "";
    }

    auto m11csv = [&](const QString &tag,
                      const MeasurementSnapshot::Meteo11Export &m, bool incl) {
        if (!incl) return;
        out << csvQ(S, {tag, ""});
        if (m.valid) {
            out << csvQ(S, {"bulletin", m.bulletinString});
            out << csvQ(S, {"NNNNN",    m.stationNumber});
            out << csvQ(S, {"BBBB",     QString::number(m.stationAltitude)});
            out << csvQ(S, {"BBB",      QString::number(m.pressureDev)});
            out << csvQ(S, {"T0T0",     QString::number(m.tempVirtDev)});
            out << csvQ(S, {"reached_temp_km", QString::number(m.reachedTempKm)});
            out << csvQ(S, {"reached_wind_km", QString::number(m.reachedWindKm)});
        } else { out << csvQ(S, {"","нет данных"}); }
        out << "";
    };
    m11csv("[МЕТЕО-11 УТОЧНЁННЫЙ]",       s.meteo11Updated,     o.includeMeteo11Updated);
    m11csv("[МЕТЕО-11 ПРИБЛИЖЁННЫЙ]",     s.meteo11Approximate, o.includeMeteo11Approx);
    m11csv("[МЕТЕО-11 ОТ МЕТЕОСТАНЦИИ]",  s.meteo11FromStation, o.includeMeteo11Station);

    out << csvQ(S, {"[СФОРМИРОВАНО]",
                    QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")});
    return out.join('\n');
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::generateJson(const MeasurementSnapshot &s,
                                          const ExportOptions       &o)
{
    QJsonObject root;

    QJsonObject meta;
    meta["record_id"]      = s.recordId;
    meta["datetime"]       = s.measurementTime.toString(Qt::ISODate);
    meta["station_number"] = s.stationNumber;
    meta["exported_at"]    = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["meta"] = meta;

    if (o.includeCoordinates) {
        QJsonObject c;
        c["valid"] = s.coordinatesValid;
        if (s.coordinatesValid) {
            c["latitude"]   = s.latitude;
            c["longitude"]  = s.longitude;
            c["altitude_m"] = s.altitude;
        }
        root["coordinates"] = c;
    }

    if (o.includeSurfaceMeteo) {
        QJsonObject sm;
        sm["valid"] = s.surfaceMeteoValid;
        if (s.surfaceMeteoValid) {
            sm["pressure_mmhg"]     = s.pressureMmHg;
            sm["temperature_c"]    = s.temperatureC;
            sm["humidity_pct"]     = s.humidityPct;
            sm["wind_dir_deg"]     = s.surfaceWindDir;
            sm["wind_speed_ms"]    = s.surfaceWindSpeed;
        }
        root["surface_meteo"] = sm;
    }

    auto windArr = [](const QVector<WindProfileData> &data) {
        QJsonArray arr;
        for (const WindProfileData &p : data) {
            QJsonObject o;
            o["h"]   = p.height;
            o["spd"] = p.windSpeed;
            o["dir"] = p.windDirection;
            arr.append(o);
        }
        return arr;
    };

    if (o.includeAvgWind) {
        QJsonObject w; w["count"] = s.avgWind.size(); w["data"] = windArr(s.avgWind);
        root["avg_wind"] = w;
    }
    if (o.includeActualWind) {
        QJsonObject w; w["count"] = s.actualWind.size(); w["data"] = windArr(s.actualWind);
        root["actual_wind"] = w;
    }
    if (o.includeMeasuredWind) {
        QJsonArray arr;
        for (const MeasuredWindData &p : s.measuredWind) {
            QJsonObject o;
            o["h"] = p.height; o["spd"] = p.windSpeed;
            o["dir"] = p.windDirection; o["rel"] = p.reliability;
            arr.append(o);
        }
        QJsonObject w; w["count"] = s.measuredWind.size(); w["data"] = arr;
        root["measured_wind"] = w;
    }
    if (o.includeWindShear) {
        QJsonArray arr;
        for (const WindShearData &w : s.windShear) {
            QJsonObject o;
            o["h"]        = w.height;
            o["shear"]    = w.shearPer30m;
            o["dir_chg"]  = w.shearDirection;
            o["severity"] = w.severityLevel;
            o["sev_text"] = WindShearCalculator::getSeverityText(w.severityLevel);
            arr.append(o);
        }
        QJsonObject sh; sh["count"] = s.windShear.size(); sh["data"] = arr;
        root["wind_shear"] = sh;
    }

    auto m11js = [](const MeasurementSnapshot::Meteo11Export &m) {
        QJsonObject o;
        o["valid"] = m.valid;
        if (m.valid) {
            o["bulletin"]         = m.bulletinString;
            o["station_number"]   = m.stationNumber;
            o["BBBB"]             = m.stationAltitude;
            o["BBB_pressure_dev"] = m.pressureDev;
            o["T0T0_temp_dev"]    = m.tempVirtDev;
            o["reached_temp_km"]  = m.reachedTempKm;
            o["reached_wind_km"]  = m.reachedWindKm;
        }
        return o;
    };
    if (o.includeMeteo11Updated)  root["meteo11_updated"]     = m11js(s.meteo11Updated);
    if (o.includeMeteo11Approx)   root["meteo11_approximate"] = m11js(s.meteo11Approximate);
    if (o.includeMeteo11Station)  root["meteo11_from_station"]= m11js(s.meteo11FromStation);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}


// ─────────────────────────────────────────────────────────────────────────────
// PDF Helpers - HTML
// ─────────────────────────────────────────────────────────────────────────────

static const char *kCss =
    "body{font-family:Arial,Helvetica,sans-serif;font-size:9pt;color:#111;margin:0;padding:0}"
    "h1{font-size:13pt;color:#1565C0;margin:0 0 4px}"
    ".hdr{background:#1565C0;color:#fff;padding:10px 14px;margin-bottom:14px}"
    ".hdr .sub{font-size:8pt;opacity:.85;margin-top:3px}"
    "h2{font-size:10pt;color:#1565C0;border-bottom:1.5px solid #1565C0;"
    "padding-bottom:2px;margin:14px 0 6px}"
    "table{border-collapse:collapse;width:100%;font-size:8.5pt}"
    "th{background:#1976D2;color:#fff;padding:4px 8px;text-align:center;"
    "border:1px solid #1565C0}"
    "td{padding:3px 8px;text-align:center;border:1px solid #ccc}"
    "tr:nth-child(even) td{background:#EEF4FB}"
    ".kv{width:100%;border-collapse:collapse;font-size:9pt;margin-bottom:6px}"
    ".kv td{border:none;padding:2px 6px}"
    ".kv .k{font-weight:bold;width:46%;color:#333}"
    ".kv .v{color:#111}"
    ".bulletin{font-family:‘Courier New’,monospace;font-size:9.5pt;"
    "background:#F5F5F5;padding:8px 10px;border-left:3px solid #1976D2;"
    "word-break:break-all;margin-bottom:6px}"
    ".nodata{color:#999;font-style:italic}"
    ".chart{width:100%;margin:6px 0 10px}"
    ".sev1{background:#C8E6C9} .sev2{background:#FFF9C4}"
    ".sev3{background:#FFCCBC} .sev4{background:#FFCDD2}";

static QString chartImg(const QMap<QString, QImage> &charts,
                        const QString &key, bool include)
{
    if (!include || !charts.contains(key) || charts[key].isNull()) return {};
    return QString("<img class=\"chart\" src=\"img://%1\"/>").arg(key);
}

QString MeasurementExporter::buildHtmlReport(const MeasurementSnapshot &s,
                                             const ExportOptions       &o)
{
    QString html;
    html.reserve(32768);

    html += "<!DOCTYPE html><html><head>"
            "<meta charset=\"UTF-8\"/>"
            "<style>" + QString(kCss) + "</style>"
                              "</head><body>";

    // Шапка
    html += "<div class=\"hdr\">"
            "<h1>Результаты измерений — Метеокомплекс</h1>"
            "<div class=\"sub\">"
            "Запись ID: <b>" + QString::number(s.recordId) + "</b>&nbsp;&nbsp;|&nbsp;&nbsp;"
                                            "Дата/время: <b>" + s.measurementTime.toString("dd.MM.yyyy hh:mm:ss") + "</b>";
    if (!s.stationNumber.isEmpty())
        html += "&nbsp;&nbsp;|&nbsp;&nbsp;Ст.: <b>" + s.stationNumber + "</b>";
    html += "</div></div>";

    // Координаты + наземные условия (в одну строку)
    if ((o.includeCoordinates && s.coordinatesValid) ||
        (o.includeSurfaceMeteo && s.surfaceMeteoValid))
    {
        html += "<table style=\"width:100%;border-collapse:collapse\"><tr>";
        if (o.includeCoordinates && s.coordinatesValid) {
            html += "<td style=\"vertical-align:top;width:50%;padding-right:10px\">"
                    "<h2>Координаты станции</h2>"
                    "<table class=\"kv\">"
                    "<tr><td class=\"k\">Широта</td><td class=\"v\">"  + latToStr(s.latitude)  + "</td></tr>"
                                             "<tr><td class=\"k\">Долгота</td><td class=\"v\">" + lonToStr(s.longitude) + "</td></tr>"
                                              "<tr><td class=\"k\">Высота, м</td><td class=\"v\">" +
                    QString::number(s.altitude,'f',1) + "</td></tr>"
                                                          "</table></td>";
        }
        if (o.includeSurfaceMeteo && s.surfaceMeteoValid) {
            html += "<td style=\"vertical-align:top\">"
                    "<h2>Наземные метеоусловия</h2>"
                    "<table class=\"kv\">"
                    "<tr><td class=\"k\">Давление, мм рт.ст.</td><td class=\"v\">"  +
                    QString::number(s.pressureMmHg,     'f',1) + "</td></tr>"
                                                             "<tr><td class=\"k\">Температура, °C</td><td class=\"v\">"      +
                    QString::number(s.temperatureC,    'f',1) + "</td></tr>"
                                                              "<tr><td class=\"k\">Влажность, %</td><td class=\"v\">"         +
                    QString::number(s.humidityPct,     'f',1) + "</td></tr>"
                                                             "<tr><td class=\"k\">Направление ветра, °</td><td class=\"v\">" +
                    QString::number(s.surfaceWindDir,  'f',0) + "</td></tr>"
                                                                "<tr><td class=\"k\">Скорость ветра, м/с</td><td class=\"v\">"  +
                    QString::number(s.surfaceWindSpeed,'f',1) + "</td></tr>"
                                                                  "</table></td>";
        }
        html += "</tr></table>";
    }

    // Средний ветер
    if (o.includeAvgWind) {
        html += "<h2>Средний ветер</h2>";
        if (!s.avgWind.isEmpty()) {
            // Графики рядом
            if (o.pdfCharts) {
                html += "<table style=\"width:100%\"><tr>"
                        "<td>" + chartImg(s.charts, "avgSpeed", true) + "</td>"
                                                                 "<td>" + chartImg(s.charts, "avgDir",   true) + "</td>"
                                                               "</tr></table>";
            }
            html += htmlWindTable(s.avgWind);
        } else { html += "<p class=\"nodata\">— нет данных —</p>"; }
    }

    // Действительный ветер
    if (o.includeActualWind) {
        html += "<h2>Действительный ветер</h2>";
        if (!s.actualWind.isEmpty()) {
            if (o.pdfCharts) {
                html += "<table style=\"width:100%\"><tr>"
                        "<td>" + chartImg(s.charts, "actualSpeed", true) + "</td>"
                                                                    "<td>" + chartImg(s.charts, "actualDir",   true) + "</td>"
                                                                  "</tr></table>";
            }
            html += htmlWindTable(s.actualWind);
        } else { html += "<p class=\"nodata\">— нет данных —</p>"; }
    }

    // Измеренный ветер
    if (o.includeMeasuredWind) {
        html += "<h2>Измеренный ветер</h2>";
        if (!s.measuredWind.isEmpty()) {
            if (o.pdfCharts) {
                html += "<table style=\"width:100%\"><tr>"
                        "<td>" + chartImg(s.charts, "measSpeed", true) + "</td>"
                                                                  "<td>" + chartImg(s.charts, "measDir",   true) + "</td>"
                                                                "</tr></table>";
            }
            html += htmlMeasTable(s.measuredWind);
        } else { html += "<p class=\"nodata\">— нет данных —</p>"; }
    }

    // Сдвиг ветра
    if (o.includeWindShear) {
        html += "<h2>Сдвиг ветра</h2>";
        if (!s.windShear.isEmpty()) {
            if (o.pdfCharts) {
                html += "<table style=\"width:100%\"><tr>"
                        "<td>" + chartImg(s.charts, "shearSpeed", true) + "</td>"
                                                                   "<td>" + chartImg(s.charts, "shearDir",   true) + "</td>"
                                                                 "</tr></table>";
            }
            html += htmlShearTable(s.windShear);
        } else { html += "<p class=\"nodata\">— нет данных —</p>"; }
    }

    // Метео-11
    if (o.includeMeteo11Updated)
        html += htmlMeteo11Block(s.meteo11Updated,     "Бюллетень Метео-11 (уточнённый)");
    if (o.includeMeteo11Approx)
        html += htmlMeteo11Block(s.meteo11Approximate, "Бюллетень Метео-11 (приближённый)");
    if (o.includeMeteo11Station)
        html += htmlMeteo11Block(s.meteo11FromStation, "Бюллетень Метео-11 (от метеостанции)");

    // Подвал
    html += "<hr style=\"border:none;border-top:1px solid #ccc;margin-top:14px\"/>"
            "<p style=\"font-size:7.5pt;color:#888;text-align:right\">"
            "Сформировано: " +
            QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") +
            " &nbsp;|&nbsp; Метеокомплекс АМС</p>";

    html += "</body></html>";
    return html;
}

QString MeasurementExporter::htmlWindTable(const QVector<WindProfileData> &data)
{
    QString t = "<table><tr><th>Высота, м</th><th>Скорость, м/с</th>"
                "<th>Направление, °</th></tr>";
    for (const WindProfileData &p : data)
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                 .arg(static_cast<int>(p.height))
                 .arg(p.windSpeed,    0,'f',2)
                 .arg(p.windDirection);
    return t + "</table>";
}

QString MeasurementExporter::htmlMeasTable(const QVector<MeasuredWindData> &data)
{
    QString t = "<table><tr><th>Высота, м</th><th>Скорость, м/с</th>"
                "<th>Направление, °</th></tr>";
    for (const MeasuredWindData &p : data)
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td></tr>")
                 .arg(static_cast<int>(p.height))
                 .arg(p.windSpeed,    0,'f',2)
                 .arg(p.windDirection);
    return t + "</table>";
}

QString MeasurementExporter::htmlShearTable(const QVector<WindShearData> &data)
{
    QString t = "<table><tr><th>Высота, м</th><th>Скорость, м/с/30м</th>"
                "<th>Изм. направления, °</th><th>Уровень</th></tr>";
    for (const WindShearData &w : data) {
        QString cls;
        if      (w.severityLevel == 0) cls = " class=\"sev1\"";
        else if (w.severityLevel == 1) cls = " class=\"sev2\"";
        else if (w.severityLevel == 2) cls = " class=\"sev3\"";
        else if (w.severityLevel >= 3) cls = " class=\"sev4\"";
        t += QString("<tr%1><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                 .arg(cls)
                 .arg(static_cast<int>(w.height))
                 .arg(w.shearPer30m,    0,'f',2)
                 .arg(w.shearDirection, 0,'f',1)
                 .arg(WindShearCalculator::getSeverityText(w.severityLevel).toHtmlEscaped());
    }
    return t + "</table>";
}

QString MeasurementExporter::htmlMeteo11Block(
    const MeasurementSnapshot::Meteo11Export &m, const QString &title)
{
    QString html = "<h2>" + title.toHtmlEscaped() + "</h2>";
    if (!m.valid) return html + "<p class=\"nodata\">— нет данных —</p>";

    html += "<div class=\"bulletin\">" + m.bulletinString.toHtmlEscaped() + "</div>";
    html += "<table class=\"kv\">"
            "<tr><td class=\"k\">Номер ст. (NNNNN)</td><td class=\"v\">" + m.stationNumber + "</td></tr>"
                                "<tr><td class=\"k\">BBBB (высота ст.)</td><td class=\"v\">"
            + QString("%1").arg(m.stationAltitude,4,10,QChar('0')) + "</td></tr>"
                                                                        "<tr><td class=\"k\">БББ (откл. давл.)</td><td class=\"v\">"
            + QString("%1").arg(m.pressureDev,3,10,QChar('0')) + "</td></tr>"
                                                                    "<tr><td class=\"k\">ТТ (откл. темп.)</td><td class=\"v\">"
            + QString("%1").arg(m.tempVirtDev,2,10,QChar('0')) + "</td></tr>"
                                                                    "<tr><td class=\"k\">Выс. темп. зонд., км</td><td class=\"v\">"
            + QString::number(m.reachedTempKm) + "</td></tr>"
                                                 "<tr><td class=\"k\">Выс. ветр. зонд., км</td><td class=\"v\">"
            + QString::number(m.reachedWindKm) + "</td></tr>"
                                                 "</table>";
    return html;

}

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные методы
// ─────────────────────────────────────────────────────────────────────────────

QString MeasurementExporter::latToStr(double lat)
{
    double absVal = qAbs(lat);
    int    deg    = static_cast<int>(absVal);
    double minF   = (absVal - deg) * 60.0;
    int    min    = static_cast<int>(minF);
    double sec    = (minF - min) * 60.0;
    return QString("%1°%2'%3\" %4")
        .arg(deg)
        .arg(min,  2, 10, QChar('0'))
        .arg(sec,  5, 'f', 2, QChar('0'))
        .arg(lat >= 0 ? "С" : "Ю");
}

QString MeasurementExporter::lonToStr(double lon)
{
    double absVal = qAbs(lon);
    int    deg    = static_cast<int>(absVal);
    double minF   = (absVal - deg) * 60.0;
    int    min    = static_cast<int>(minF);
    double sec    = (minF - min) * 60.0;
    return QString("%1°%2'%3\" %4")
        .arg(deg)
        .arg(min,  2, 10, QChar('0'))
        .arg(sec,  5, 'f', 2, QChar('0'))
        .arg(lon >= 0 ? "В" : "З");
}

QString MeasurementExporter::formatExt(ExportOptions::Format fmt)
{
    switch (fmt) {
    case ExportOptions::TXT:  return "txt";
    case ExportOptions::CSV:  return "csv";
    case ExportOptions::JSON: return "json";
    case ExportOptions::PDF: return "pdf";
    case ExportOptions::XLSX: return "xlsx";
    }
    return "txt";
}