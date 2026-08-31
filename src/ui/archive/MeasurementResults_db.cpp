// ─────────────────────────────────────────────────────────────────────────
// Чтение из базы: список измерений, профили ветра, приземные данные,
// координаты станции и исходный бюллетень Метео-11 от метеостанции.
//
// Часть реализации класса MeasurementResults (см. MeasurementResults.h).
// Общий для всех частей набор include — в MeasurementResults_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/archive/MeasurementResults_internal.h"

void MeasurementResults::clearStationCoordinates()
{
    ui->valLatitude->setText("—");
    ui->valLongitude->setText("—");
    ui->valAltitude->setText("—");
    m_stationCoordsValid = false;
}

// ===== НАСТРОЙКА БД =====

//void MeasurementResults::setDatabase(const QString &host, int port, const QString &dbName,
//                                     const QString &user, const QString &password)
//{
//    DatabaseManager::instance()->configure(host, port, dbName, user, password);
//    DatabaseManager::instance()->connect();

//    qInfo() << "MeasurementResults: Использую подключение к БД";

//    // Загружаем доступные измерения
//    loadAvailableMeasurements();
//}

bool MeasurementResults::connectDatabase()
{
    if(!DatabaseManager::instance()->isConnected()){
        return DatabaseManager::instance()->connect();
    }
    return true;
}

void MeasurementResults::disconnectDatabase()
{

}

// ===== ЗАГРУЗКА ДАННЫХ ИЗ БД =====

void MeasurementResults::loadMeasurementsFromDatabase()
{
    if (!DatabaseManager::instance()->isConnected()) {
        qWarning() << "MeasurementResults: БД не подключена";
        if (!DatabaseManager::instance()->connect()) {
            qCritical() << "MeasurementResults: Не удалось подключиться к БД";
            return;
        }
    }

    availableMeasurements.clear();

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);

    // Загружаем ВСЕ записи архива (без ограничения по дате — требование
    // хранения не менее года). Наличие профилей ветра определяется тем же
    // запросом через LEFT JOIN на wind_profiles_references — это убирает
    // N отдельных запросов в цикле и делает открытие архива быстрым даже
    // на больших объёмах.
    //
    // CASE WHEN ... IS NOT NULL — флаг наличия соответствующего профиля.
    QString sql =
        "SELECT "
        "   ma.record_id, "
        "   ma.completion_time, "
        "   ma.notes, "
        "   (wpr.avg_wind_profile_id      IS NOT NULL) AS has_avg, "
        "   (wpr.actual_wind_profile_id   IS NOT NULL) AS has_actual, "
        "   (wpr.measured_wind_profile_id IS NOT NULL) AS has_measured "
        "FROM main_archive ma "
        "LEFT JOIN wind_profiles_references wpr "
        "       ON wpr.record_id = ma.record_id "
        "ORDER BY ma.completion_time DESC";

    qDebug() << "MeasurementResults: Выполняем запрос к main_archive (весь архив)...";

    if (!query.exec(sql)) {
        qCritical() << "MeasurementResults: Ошибка SQL:" << query.lastError().text();
        qDebug() << "SQL запрос:" << sql;
        return;
    }

    int totalRecords = 0;

    while (query.next()) {
        MeasurementRecord record;
        record.recordId        = query.value(0).toInt();
        record.measurementTime = query.value(1).toDateTime();
        record.notes           = query.value(2).toString();

        // Флаги наличия профилей пришли тем же запросом — без доп. обращений к БД
        record.hasAvgWind      = query.value(3).toBool();
        record.hasActualWind   = query.value(4).toBool();
        record.hasMeasuredWind = query.value(5).toBool();

        const QDate date = record.measurementTime.date();
        availableMeasurements[date].append(record);
        totalRecords++;
    }

    qInfo() << "MeasurementResults: Загружено" << totalRecords
            << "записей из main_archive (весь архив)";

    // Сортируем записи внутри каждой даты по времени (новые сверху)
    for (auto it = availableMeasurements.begin(); it != availableMeasurements.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const MeasurementRecord &a, const MeasurementRecord &b) {
                      return a.measurementTime > b.measurementTime;
                  });
    }

    qInfo() << "MeasurementResults: Данные распределены по"
            << availableMeasurements.size() << "датам";

    // Список дат для отладки
    QList<QDate> dates = availableMeasurements.keys();
    std::sort(dates.begin(), dates.end());
    qDebug() << "Доступные даты в архиве:" << dates;
}

QVector<WindProfileData> MeasurementResults::loadAvgWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT avg_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет avg_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM avg_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки среднего ветра:" << query.lastError().text();
        return profile;
    }

    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.isValid       = true;
        dbData.append(point);
    }

    profile = dbData;
    QStringList heightList;
    for (const auto &pt : profile)
        heightList << QString::number(qRound(pt.height));
    qDebug() << "MeasurementResults: Средний ветер record_id=" << recordId
             << "profile_id=" << profileId
             << "точек=" << profile.size()
             << "высоты:" << heightList.join(", ");
    return profile;
}

QVector<WindProfileData> MeasurementResults::loadActualWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT actual_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет actual_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction "
        "FROM actual_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки действительного ветра:" << query.lastError().text();
        return profile;
    }

    QVector<WindProfileData> dbData;
    while (query.next()) {
        WindProfileData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        point.isValid       = true;
        dbData.append(point);
    }

    profile = dbData;
    QStringList heightListA;
    for (const auto &pt : profile)
        heightListA << QString::number(qRound(pt.height));
    qDebug() << "MeasurementResults: Действительный ветер record_id=" << recordId
             << "profile_id=" << profileId
             << "точек=" << profile.size()
             << "высоты:" << heightListA.join(", ");
    return profile;
}

QVector<MeasuredWindData> MeasurementResults::loadMeasuredWindProfile(int recordId)
{
    QVector<MeasuredWindData> profile;

    if (recordId <= 0 || !connectDatabase()) return profile;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем profile_id из wind_profiles_references
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT measured_wind_profile_id FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next() || refQuery.value(0).isNull()) {
        qDebug() << "MeasurementResults: Нет measured_wind_profile для record_id=" << recordId;
        return profile;
    }

    int profileId = refQuery.value(0).toInt();

    QSqlQuery query(db);
    query.prepare(
        "SELECT height, wind_speed, wind_direction, reliability "
        "FROM measured_wind_profile "
        "WHERE profile_id = :pid "
        "ORDER BY height ASC"
        );
    query.bindValue(":pid", profileId);

    if (!query.exec()) {
        qCritical() << "MeasurementResults: Ошибка загрузки измеренного ветра:" << query.lastError().text();
        return profile;
    }

    while (query.next()) {
        MeasuredWindData point;
        point.height        = query.value(0).toFloat();
        point.windSpeed     = query.value(1).toFloat();
        point.windDirection = query.value(2).toInt();
        // Достоверность от АМС (1 - достоверная, 0 - недостоверная)
        point.reliability   = query.value(3).toInt();
        profile.append(point);
    }
    qDebug() << "MeasurementResults: Загружен профиль измеренного ветра," << profile.size()
             << "точек (record_id=" << recordId << ", profile_id=" << profileId << ")";
    return profile;
}

void MeasurementResults::loadSurfaceMeteoData(int recordId)
{
    ui->tableWidget_parm1b65->clearContents();
    if (recordId <= 0 || !connectDatabase()) return;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT temperature, humidity, pressure, wind_speed_surface, wind_direction_surface "
        "FROM surface_meteo WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет данных ИВС для record_id=" << recordId;
        return;
    }

    auto setCell = [&](int row, const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->tableWidget_parm1b65->setItem(row, 0, item);
    };

    setCell(0, QString::number(query.value(2).toDouble(), 'f', 1)); // давление
    setCell(1, QString::number(query.value(0).toDouble(), 'f', 1)); // температура
    setCell(2, QString::number(query.value(1).toDouble(), 'f', 1)); // влажность
    setCell(3, QString::number(query.value(4).toInt(), 10));         // направление
    setCell(4, QString::number(query.value(3).toDouble(), 'f', 1)); // скорость

    // Сохраняем значения для последующего формирования Метео-11
    m_currentPressureMmHg     = query.value(2).toDouble(); // из БД уже в мм рт.ст.
    m_currentTempC            = query.value(0).toDouble();
    m_currentWindDirSurface   = query.value(4).toDouble();
    m_currentWindSpeedSurface = query.value(3).toDouble();
}

void MeasurementResults::loadStationCoordinates(int recordId)
{
    if (recordId <= 0 || !connectDatabase()) return;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT latitude, longitude, altitude "
        "FROM station_coordinates WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет координат для record_id=" << recordId;
        clearStationCoordinates();
        return;
    }

    double lat = query.value(0).toDouble();
    double lon = query.value(1).toDouble();
    double alt = query.value(2).toDouble();

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
