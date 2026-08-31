// ─────────────────────────────────────────────────────────────────────────
// Выгрузка результатов: сбор снимка измерения и передача его
// в MeasurementExporter (см. core/export).
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

void MeasurementResults::onExportBackRequested()
{
    ui->rootStack->setCurrentWidget(ui->archivePage);
}

MeasurementSnapshot MeasurementResults::buildSnapshot() const
{
    MeasurementSnapshot snap;

    // Определяем текущую запись (та же логика, что в loadMeasurementData)
    MeasurementRecord record;
    QDate date = currentDateTime.date();
    if (availableMeasurements.contains(date)) {
        for (const MeasurementRecord &r : availableMeasurements[date]) {
            if (r.measurementTime == currentDateTime) { record = r; break; }
        }
    }
    if (record.recordId <= 0)
        record = const_cast<MeasurementResults*>(this)
                     ->findClosestRecord(date, currentDateTime.time().hour());

    snap.recordId        = record.recordId;
    snap.measurementTime = (record.recordId > 0)
                               ? record.measurementTime
                               : currentDateTime;
    snap.stationNumber   = ui->lineEdit_numStation->text().trimmed();

    if (snap.recordId <= 0)
        return snap;   // Нет загруженных данных — возвращаем пустой снимок

    // ── Координаты ───────────────────────────────────────────────────────────
    // Используем кешированные значения из loadStationCoordinates()
    // (без вызова CoordHelper::fromDisplayDMS — см. ШАГ 3)
    snap.coordinatesValid = m_stationCoordsValid;
    if (snap.coordinatesValid) {
        snap.latitude  = m_currentLatitude;
        snap.longitude = m_currentLongitude;
        snap.altitude  = m_currentStationAltitude;
    }

    // ── Наземные метеоусловия ────────────────────────────────────────────────
    {
        QTableWidget *t = ui->tableWidget_parm1b65;
        snap.surfaceMeteoValid = (t->item(0, 0) != nullptr &&
                                  !t->item(0, 0)->text().isEmpty());
        if (snap.surfaceMeteoValid) {
            // Строки в tableWidget_parm1b65: 0=давление, 1=темп, 2=влажность,
            //                                3=направление, 4=скорость
            auto cell = [&](int row) -> double {
                return t->item(row, 0) ? t->item(row, 0)->text().toDouble() : 0.0;
            };
            snap.pressureHpa      = cell(0);
            snap.temperatureC     = cell(1);
            snap.humidityPct      = cell(2);
            snap.surfaceWindDir   = cell(3);
            snap.surfaceWindSpeed = cell(4);
        }
    }

    // ── Профили ветра ────────────────────────────────────────────────────────
    // Загружаем из БД (лёгкий повторный запрос — данные кешируются на уровне БД)
    snap.avgWind      = m_currentAvgWind;
    snap.actualWind   = m_currentActualWind;
    snap.measuredWind = m_currentMeasuredWind;

    // ── Сдвиг ветра ──────────────────────────────────────────────────────────
    snap.windShear = m_currentShearData;

    // ── Рендеринг графиков в QImage для PDF ──────────────────────────────────
    auto renderPlot = [](QwtPlot *plot, int w, int h) -> QImage {
        if (!plot) return {};
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(Qt::white);
        QPainter painter(&img);
        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, false);
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasFrame,  false);
        renderer.render(plot, &painter, QRectF(img.rect()));
        return img;
    };

    const int CW = 400, CH = 280;
    snap.charts["avgSpeed"]    = renderPlot(ui->plot_midWindSpeed,       CW, CH);
    snap.charts["avgDir"]      = renderPlot(ui->plot_midWindAzimut,      CW, CH);
    snap.charts["actualSpeed"] = renderPlot(ui->plot_realWindSpeed,      CW, CH);
    snap.charts["actualDir"]   = renderPlot(ui->plot_realWindAzimut,     CW, CH);
    snap.charts["measSpeed"]   = renderPlot(ui->plot_izmWindSpeed_2,     CW, CH);
    snap.charts["measDir"]     = renderPlot(ui->plot_izmWindAzimut_2,    CW, CH);
    snap.charts["shearSpeed"]  = renderPlot(ui->plot_windShearSpeed,     CW, CH);
    snap.charts["shearDir"]    = renderPlot(ui->plot_windShearDirection, CW, CH);

    // ── Метео-11 ─────────────────────────────────────────────────────────────
    auto copyM11 = [](const MeasurementResults::Meteo11Data        &src,
                      MeasurementSnapshot::Meteo11Export            &dst) {
        dst.valid           = src.isValid;
        dst.bulletinString  = src.isValid
                                 ? MeasurementResults::buildMeteo11String(src)
                                 : QString();
        dst.stationNumber   = src.stationNumber;
        dst.day             = src.day;
        dst.hour            = src.hour;
        dst.tenMinutes      = src.tenMinutes;
        dst.stationAltitude = src.stationAltitude;
        dst.pressureDev     = src.pressureDeviation;
        dst.tempVirtDev     = src.tempVirtualDev;
        dst.reachedTempKm   = src.reachedTempHeightKm;
        dst.reachedWindKm   = src.reachedWindHeightKm;
    };

    copyM11(m_meteo11Updated,     snap.meteo11Updated);
    copyM11(m_meteo11Approximate, snap.meteo11Approximate);
    copyM11(m_meteo11FromStation, snap.meteo11FromStation);

    return snap;

}

// ─────────────────────────────────────────────────────────────────────────────
// Слот кнопки «Экспорт»
// ─────────────────────────────────────────────────────────────────────────────
void MeasurementResults::onExportClicked()
{
    // Собираем снимок данных и переключаемся на встроенный экран экспорта
    // (страница 1 rootStack) вместо модального ExportDialog.
    MeasurementSnapshot snap = buildSnapshot();

    if (snap.recordId <= 0) {
        showStatus("Нет данных для экспорта: выберите дату и время с доступными измерениями, "
                   "чтобы данные были загружены из архива.", NotificationToast::Error);
        return;
    }

    m_exportView->setSnapshot(snap);
    ui->rootStack->setCurrentWidget(m_exportView);
}

void MeasurementResults::onExportSubmitted(const MeasurementSnapshot &snap, const ExportOptions &opts)
{
    // Диалог выбора пути
    const struct { ExportOptions::Format fmt; const char *filter; } kFilters[] = {
                    { ExportOptions::TXT,  "Текстовый файл (*.txt);;Все файлы (*)"  },
                    { ExportOptions::CSV,  "CSV файл (*.csv);;Все файлы (*)"        },
                    { ExportOptions::JSON, "JSON файл (*.json);;Все файлы (*)"      },
                    { ExportOptions::PDF,  "PDF файл (*.pdf);;Все файлы (*)"        },
                    { ExportOptions::XLSX, "Excel файл (*.xlsx);;Все файлы (*)"     },
                    };
    QString filter;
    for (const auto &kf : kFilters)
        if (kf.fmt == opts.format) { filter = kf.filter; break; }

    QString defaultName = MeasurementExporter::suggestedFileName(snap, opts.format);
    QString path = QFileDialog::getSaveFileName(
        this,
        "Сохранить результаты измерений",
        QDir::homePath() + QDir::separator() + defaultName,
        filter);

    if (path.isEmpty())
        return;

    // 4. Сохранение
    bool ok = false;
    QString errorMsg;

    if (opts.format == ExportOptions::PDF) {
        ok = MeasurementExporter::generatePdf(snap, opts, path, errorMsg);
    }
    else if (opts.format == ExportOptions::XLSX) {
        ok = MeasurementExporter::generateXlsx(snap, opts, path, errorMsg);
    }
    else {
        // TXT / CSV / JSON
        QString content = MeasurementExporter::generate(snap, opts, errorMsg);
        if (!errorMsg.isEmpty()) {
            showStatus("Ошибка экспорта: " + errorMsg, NotificationToast::Error);
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            showStatus(QString("Не удалось открыть файл: %1. Ошибка: %2")
                           .arg(path, file.errorString()),
                       NotificationToast::Error);
            return;
        }

        QTextStream stream(&file);
        // Совместимость Qt5 / Qt6

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        stream.setEncoding(QStringConverter::Utf8);
#else
        stream.setCodec("UTF-8");
#endif
        stream << content;
        file.close();
        ok = true;
    }

    // 5. Результат
    if (!ok) {
        if (!errorMsg.isEmpty())
            showStatus("Ошибка экспорта: " + errorMsg, NotificationToast::Error);
        return;
    }

    // Успешный экспорт — возвращаемся к архиву, чтобы не держать пользователя
    // на экране экспорта; при ошибке остаёмся, чтобы можно было поправить опции.
    ui->rootStack->setCurrentWidget(ui->archivePage);

    m_toast->showMessageWithAction(
        QString("Файл успешно сохранён: %1").arg(QFileInfo(path).fileName()),
        NotificationToast::Success,
        "Открыть папку",
        [path]() {
            QDesktopServices::openUrl(
                QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
        });
}