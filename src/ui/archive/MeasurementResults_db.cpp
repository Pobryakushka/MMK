// ─────────────────────────────────────────────────────────────────────────
// Загрузка данных измерения на страницу архива.
//
// Сами запросы к базе живут в data/MeasurementRepository — здесь остался
// только интерфейс: попросить у репозитория готовые структуры и разложить их
// по таблицам, подписям и полям страницы. Виджет больше не подключает
// <QSqlQuery> и не знает ни имён таблиц, ни имён столбцов.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

#include "data/MeasurementRepository.h"

void MeasurementResults::clearStationCoordinates()
{
    ui->valLatitude->setText("—");
    ui->valLongitude->setText("—");
    ui->valAltitude->setText("—");
    m_stationCoordsValid = false;
}

// ===== НАСТРОЙКА БД =====

bool MeasurementResults::connectDatabase()
{
    return MeasurementRepository::ensureConnected();
}

void MeasurementResults::disconnectDatabase()
{

}

// ===== ЗАГРУЗКА ДАННЫХ ИЗ БД =====

void MeasurementResults::loadMeasurementsFromDatabase()
{
    // Подключение проверяется отдельным шагом: при недоступной БД уже
    // загруженный список измерений сохраняется, а при неудачном запросе —
    // сбрасывается. Так было и до выноса SQL в репозиторий.
    if (!MeasurementRepository::ensureConnectedLogged())
        return;

    availableMeasurements.clear();

    QVector<MeasurementRecord> records;
    if (!MeasurementRepository::loadAllRecords(records))
        return;

    for (const MeasurementRecord &record : records) {
        const QDate date = record.measurementTime.date();
        availableMeasurements[date].append(record);
    }

    qInfo() << "MeasurementResults: Данные распределены по"
            << availableMeasurements.size() << "датам";

    // Сортируем записи внутри каждой даты по времени (новые сверху)
    for (auto it = availableMeasurements.begin(); it != availableMeasurements.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const MeasurementRecord &a, const MeasurementRecord &b) {
                      return a.measurementTime > b.measurementTime;
                  });
    }

    // Список дат для отладки
    QList<QDate> dates = availableMeasurements.keys();
    std::sort(dates.begin(), dates.end());
    qDebug() << "Доступные даты в архиве:" << dates;
}

QVector<WindProfileData> MeasurementResults::loadAvgWindProfile(int recordId)
{
    return MeasurementRepository::loadAvgWindProfile(recordId);
}

QVector<WindProfileData> MeasurementResults::loadActualWindProfile(int recordId)
{
    return MeasurementRepository::loadActualWindProfile(recordId);
}

QVector<MeasuredWindData> MeasurementResults::loadMeasuredWindProfile(int recordId)
{
    return MeasurementRepository::loadMeasuredWindProfile(recordId);
}

void MeasurementResults::loadSurfaceMeteoData(int recordId)
{
    ui->tableWidget_parm1b65->clearContents();

    MeasurementRepository::SurfaceMeteo meteo;
    if (!MeasurementRepository::loadSurfaceMeteo(recordId, meteo))
        return;

    auto setCell = [&](int row, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->tableWidget_parm1b65->setItem(row, 0, item);
    };

    setCell(0, QString::number(meteo.pressureMmHg, 'f', 1));     // давление
    setCell(1, QString::number(meteo.temperatureC, 'f', 1));     // температура
    setCell(2, QString::number(meteo.humidityPct, 'f', 1));      // влажность
    setCell(3, QString::number(meteo.windDirSurfaceCell, 10));   // направление
    setCell(4, QString::number(meteo.windSpeedSurface, 'f', 1)); // скорость

    // Сохраняем значения для последующего формирования Метео-11
    m_currentPressureMmHg     = meteo.pressureMmHg;
    m_currentTempC            = meteo.temperatureC;
    m_currentWindDirSurface   = meteo.windDirSurface;
    m_currentWindSpeedSurface = meteo.windSpeedSurface;
}

void MeasurementResults::loadStationCoordinates(int recordId)
{
    if (recordId <= 0 || !connectDatabase()) return;

    MeasurementRepository::StationPosition pos;
    if (!MeasurementRepository::loadStationPosition(recordId, pos)) {
        clearStationCoordinates();
        return;
    }

    const double lat = pos.latitude;
    const double lon = pos.longitude;
    const double alt = pos.altitude;

    m_currentLatitude = lat;
    m_currentLongitude = lon;

    // Значения выводятся одной строкой с буквой полушария — как в макете
    // ("55°45'21\" N"), поэтому отдельные выпадающие списки больше не нужны.
    ui->valLatitude->setText(CoordHelper::toDisplayDMS(qAbs(lat)) + (lat >= 0 ? " N" : " S"));
    ui->valLongitude->setText(CoordHelper::toDisplayDMS(qAbs(lon)) + (lon >= 0 ? " E" : " W"));
    ui->valAltitude->setText(QString::number(alt, 'f', 1) + " м");
    m_stationCoordsValid = true;

    // Сохраняем высоту станции для Метео-11
    m_currentStationAltitude = alt;

    qDebug() << "MeasurementResults: Координаты загружены для record_id=" << recordId
             << "lat=" << lat << "lon=" << lon << "alt=" << alt;
}

void MeasurementResults::updateAvailableRecordsLabel()
{
    QDate date = currentDateTime.date();

    int recordCount = 0;
    if (availableMeasurements.contains(date)) {
        recordCount = availableMeasurements[date].size();
    }

    // Цвет задаётся не setStyleSheet() на самой метке (это стёрло бы остальные
    // правила для неё), а динамическим свойством, на которое есть селектор в
    // applyArchiveStyle().
    if (recordCount > 0) {
        ui->lblAvailableRecords->setText(
            QString("Доступно записей: %1 · нажмите на дату для выбора").arg(recordCount));
        setWidgetState(ui->lblAvailableRecords, "ok");
    } else {
        ui->lblAvailableRecords->setText("Нет данных за выбранную дату");
        setWidgetState(ui->lblAvailableRecords, "empty");
    }
}
