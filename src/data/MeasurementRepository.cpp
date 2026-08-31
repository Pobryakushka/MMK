#include "data/MeasurementRepository.h"

#include "data/databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QDebug>
#include <QtMath>

namespace MeasurementRepository {

bool ensureConnected()
{
    if (!DatabaseManager::instance()->isConnected()) {
        return DatabaseManager::instance()->connect();
    }
    return true;
}

bool ensureConnectedLogged()
{
    if (!DatabaseManager::instance()->isConnected()) {
        qWarning() << "MeasurementResults: БД не подключена";
        if (!DatabaseManager::instance()->connect()) {
            qCritical() << "MeasurementResults: Не удалось подключиться к БД";
            return false;
        }
    }
    return true;
}

bool loadAllRecords(QVector<MeasurementRecord> &out)
{
    out.clear();

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
        return false;
    }

    while (query.next()) {
        MeasurementRecord record;
        record.recordId        = query.value(0).toInt();
        record.measurementTime = query.value(1).toDateTime();
        record.notes           = query.value(2).toString();

        // Флаги наличия профилей пришли тем же запросом — без доп. обращений к БД
        record.hasAvgWind      = query.value(3).toBool();
        record.hasActualWind   = query.value(4).toBool();
        record.hasMeasuredWind = query.value(5).toBool();

        out.append(record);
    }

    qInfo() << "MeasurementResults: Загружено" << out.size()
            << "записей из main_archive (весь архив)";
    return true;
}

QVector<WindProfileData> loadAvgWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !ensureConnected()) return profile;

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

QVector<WindProfileData> loadActualWindProfile(int recordId)
{
    QVector<WindProfileData> profile;

    if (recordId <= 0 || !ensureConnected()) return profile;

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

QVector<MeasuredWindData> loadMeasuredWindProfile(int recordId)
{
    QVector<MeasuredWindData> profile;

    if (recordId <= 0 || !ensureConnected()) return profile;

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

bool loadSurfaceMeteo(int recordId, SurfaceMeteo &out)
{
    if (recordId <= 0 || !ensureConnected()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT temperature, humidity, pressure, wind_speed_surface, wind_direction_surface "
        "FROM surface_meteo WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет данных ИВС для record_id=" << recordId;
        return false;
    }

    out.temperatureC     = query.value(0).toDouble();
    out.humidityPct      = query.value(1).toDouble();
    out.pressureMmHg     = query.value(2).toDouble(); // из БД уже в мм рт.ст.
    out.windSpeedSurface   = query.value(3).toDouble();
    out.windDirSurface     = query.value(4).toDouble();
    out.windDirSurfaceCell = query.value(4).toInt();
    return true;
}

bool loadStationPosition(int recordId, StationPosition &out)
{
    if (recordId <= 0 || !ensureConnected()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT latitude, longitude, altitude "
        "FROM station_coordinates WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: Нет координат для record_id=" << recordId;
        return false;
    }

    out.latitude  = query.value(0).toDouble();
    out.longitude = query.value(1).toDouble();
    out.altitude  = query.value(2).toDouble();
    return true;
}

bool loadMeteo11Bulletin(int recordId, QString &bulletinJson, QDateTime &bulletinTime)
{
    if (recordId <= 0 || !ensureConnected()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "SELECT bulletin_data, bulletin_time "
        "FROM meteo_11_bulletin WHERE record_id = :rid"
        );
    query.bindValue(":rid", recordId);

    if (!query.exec() || !query.next()) {
        qDebug() << "MeasurementResults: бюллетень МС не найден для record_id=" << recordId;
        return false; // нормально — бюллетень мог не вводиться
    }

    bulletinJson = query.value(0).toString();
    bulletinTime = query.value(1).toDateTime();
    return true;
}

} // namespace MeasurementRepository
