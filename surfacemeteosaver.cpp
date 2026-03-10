#include "surfacemeteosaver.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

SurfaceMeteoSaver::SurfaceMeteoSaver(QObject *parent)
    : QObject(parent)
    , m_temperature(0.0)
    , m_humidity(0.0)
    , m_pressure(0.0)
    , m_windSpeed(0.0)
    , m_windDirection(0)
    , m_hasTemperature(false)
    , m_hasHumidity(false)
    , m_hasPressure(false)
    , m_hasWindSpeed(false)
    , m_hasWindDirection(false)
{
}

// =============================================================
// Обновление последних значений от GroundMeteoParams
// =============================================================

void SurfaceMeteoSaver::updateLastValues(const QMap<QString, double> &values)
{
    // Принимаем оба варианта названий — UMB (текущие) и Modbus (средние)
    if (values.contains("Temperature Avg")) {
        m_temperature    = values["Temperature Avg"];
        m_hasTemperature = true;
    } else if (values.contains("Temperature")) {
        m_temperature    = values["Temperature"];
        m_hasTemperature = true;
    }

    if (values.contains("Humidity Avg")) {
        m_humidity    = values["Humidity Avg"];
        m_hasHumidity = true;
    } else if (values.contains("Humidity")) {
        m_humidity    = values["Humidity"];
        m_hasHumidity = true;
    }

    if (values.contains("Pressure Avg")) {
        m_pressure    = values["Pressure Avg"];
        m_hasPressure = true;
    } else if (values.contains("Pressure")) {
        m_pressure    = values["Pressure"];
        m_hasPressure = true;
    }

    if (values.contains("Wind Speed Avg")) {
        m_windSpeed    = values["Wind Speed Avg"];
        m_hasWindSpeed = true;
    } else if (values.contains("Wind Speed")) {
        m_windSpeed    = values["Wind Speed"];
        m_hasWindSpeed = true;
    }

    if (values.contains("Wind Direction Avg")) {
        m_windDirection    = static_cast<int>(values["Wind Direction Avg"]);
        m_hasWindDirection = true;
    } else if (values.contains("Wind Direction")) {
        m_windDirection    = static_cast<int>(values["Wind Direction"]);
        m_hasWindDirection = true;
    }

    qDebug() << "SurfaceMeteoSaver: обновлены данные ИВС:"
             << "T=" << m_temperature
             << "H=" << m_humidity
             << "P=" << m_pressure
             << "WS=" << m_windSpeed
             << "WD=" << m_windDirection;
}

// =============================================================
// Сохранение в БД
// =============================================================

bool SurfaceMeteoSaver::saveToDatabase(int recordId)
{
    qDebug() << "SurfaceMeteoSaver: сохранение в surface_meteo, record_id=" << recordId;

    if (!hasData()) {
        QString error = "Нет данных от ИВС для сохранения (не получен хотя бы один цикл опроса)";
        qWarning() << "SurfaceMeteoSaver:" << error;
        emit saveError(error);
        return false;
    }

    if (!DatabaseManager::instance()->isConnected()) {
        qDebug() << "SurfaceMeteoSaver: БД не подключена, пытаемся переподключиться...";
        if (!DatabaseManager::instance()->connect()) {
            QString error = "Не удалось подключиться к БД для сохранения данных ИВС";
            qCritical() << "SurfaceMeteoSaver:" << error;
            emit saveError(error);
            return false;
        }
    }

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);

    // Проверяем — не была ли уже запись с таким record_id
    query.prepare("SELECT COUNT(*) FROM surface_meteo WHERE record_id = :rid");
    query.bindValue(":rid", recordId);
    if (query.exec() && query.next() && query.value(0).toInt() > 0) {
        qWarning() << "SurfaceMeteoSaver: запись с record_id=" << recordId
                   << "уже существует в surface_meteo, пропускаем";
        return true; // Считаем успехом — данные уже есть
    }

    query.prepare(
        "INSERT INTO surface_meteo "
        "(record_id, temperature, humidity, pressure, wind_speed_surface, wind_direction_surface) "
        "VALUES (:record_id, :temperature, :humidity, :pressure, :wind_speed, :wind_direction)"
    );

    query.bindValue(":record_id",      recordId);
    query.bindValue(":temperature",    m_temperature);
    query.bindValue(":humidity",       m_humidity);
    query.bindValue(":pressure",       m_pressure);
    query.bindValue(":wind_speed",     m_windSpeed);
    query.bindValue(":wind_direction", m_windDirection);

    if (!query.exec()) {
        QString error = QString("Ошибка INSERT в surface_meteo: %1")
                        .arg(query.lastError().text());
        qCritical() << "SurfaceMeteoSaver:" << error;
        emit saveError(error);
        return false;
    }

    qInfo() << "SurfaceMeteoSaver: данные ИВС успешно сохранены в surface_meteo, record_id=" << recordId;
    qDebug() << "  temperature=" << m_temperature
             << "  humidity=" << m_humidity
             << "  pressure=" << m_pressure
             << "  wind_speed=" << m_windSpeed
             << "  wind_direction=" << m_windDirection;

    emit savedSuccessfully(recordId);
    return true;
}

bool SurfaceMeteoSaver::hasData() const
{
    return m_hasTemperature && m_hasHumidity &&
           m_hasPressure && m_hasWindSpeed && m_hasWindDirection;
}
