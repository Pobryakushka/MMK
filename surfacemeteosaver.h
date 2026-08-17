#ifndef SURFACEMETEOSAVER_H
#define SURFACEMETEOSAVER_H

#include <QObject>
#include <QMap>
#include <QString>

/**
 * @brief Сохранение приземных метеоданных от ИВС в таблицу surface_meteo БД.
 *
 * Использование:
 *   1. По сигналу dataUpdated от GroundMeteoParams вызывать updateLastValues()
 *   2. По завершении измерения АМС вызывать saveToDatabase(recordId)
 *
 * Таблица surface_meteo:
 *   record_id              INTEGER  — совпадает с record_id из main_archive (от АМС)
 *   temperature            NUMERIC(5,2)
 *   humidity               NUMERIC(5,2)
 *   pressure               NUMERIC(7,2)
 *   wind_speed_surface     NUMERIC(5,2)
 *   wind_direction_surface INTEGER
 */
class SurfaceMeteoSaver : public QObject
{
    Q_OBJECT

public:
    explicit SurfaceMeteoSaver(QObject *parent = nullptr);

    // Обновить последние известные значения от ИВС
    // values — карта параметров как она приходит из GroundMeteoParams::dataUpdated
    void updateLastValues(const QMap<QString, double> &values);

    // Сохранить текущие last-значения в БД под указанным record_id
    // Возвращает true при успехе
    bool saveToDatabase(int recordId);

    // Проверить что все 5 параметров хотя бы раз получены
    bool hasData() const;

    // Текущие значения (для отладки / отображения)
    double temperature()    const { return m_temperature; }
    double humidity()       const { return m_humidity; }
    double pressure()       const { return m_pressure; }
    double windSpeed()      const { return m_windSpeed; }
    int    windDirection()  const { return m_windDirection; }

signals:
    void savedSuccessfully(int recordId);
    void saveError(const QString &error);

private:
    double m_temperature;
    double m_humidity;
    double m_pressure;
    double m_windSpeed;
    int    m_windDirection;

    bool m_hasTemperature;
    bool m_hasHumidity;
    bool m_hasPressure;
    bool m_hasWindSpeed;
    bool m_hasWindDirection;
};

#endif // SURFACEMETEOSAVER_H
