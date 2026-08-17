#pragma once
#include <QObject>
#include <QVector>
#include <QDateTime>
#include "GfsDownloadRunner.h"
#include "MushroomRunner.h"
#include "GribConfig.h"
#include "Meteo11Types.h"
#include "sensors/amsprotocol.h" // WindProfileData

// Полный путь "координаты -> готовый профиль ветра" одним вызовом:
//   1. GfsDownloadRunner: скрипт скачивания (--point) забирает нужный
//      маленький кусок GFS под координаты станции;
//   2. MushroomRunner: Mushroom читает файл и выдаёт сырые точки по
//      уровням давления;
//   3. Meteo11ProfileBuilder: пересчёт уровней давления в высоты AGL
//      (через приземную orog);
//   4. Meteo11Calculator: осреднение по стандартным слоям 0-200..0-8000 м;
//   5. GribWindProfileAdapter: перевод результата в WindProfileData[],
//      готовый для существующего MeasurementResults::buildMeteo11().
//
// Не зависит от UI — можно дёргать из любого места программы, не только
// из MeasurementResults. Все тяжёлые внешние процессы (bash-скрипт,
// Mushroom) выполняются асинхронно через QProcess, результат приходит
// через сигнал finished().
class GribMeteo11Pipeline : public QObject {
    Q_OBJECT
public:
    explicit GribMeteo11Pipeline(QObject *parent = nullptr);

    void setConfig(const GribConfig &config) { m_config = config; }
    const GribConfig& config() const { return m_config; }

    // lat/lon       — координаты станции (градусы)
    // sondingTime   — время зондирования; используется, чтобы выбрать
    //                 дату и ближайший (не позже) цикл GFS (00/06/12/18)
    // surfaceSpeedMs/surfaceDirectionDeg — приземный ветер с реального
    //                 датчика (то же, что используется на вкладке
    //                 "Наземн. мет. усл." для остальных типов бюллетеня)
    void run(double lat, double lon, const QDateTime &sondingTime,
             double surfaceSpeedMs, double surfaceDirectionDeg);

    // Выбор ближайшего (не позже заданного момента) цикла GFS: 00/06/12/18.
    // Публичный static — пригодится и для UI (показать пользователю,
    // какой цикл будет запрошен, до нажатия кнопки).
    static QString nearestGfsRunCycle(const QDateTime &dt);

signals:
    void logLine(const QString &line);
    // success=false -> error содержит причину, profile пуст
    void finished(bool success, const QVector<WindProfileData> &profile, const QString &error);

private:
    GribConfig m_config;
    GfsDownloadRunner m_downloadRunner;
    MushroomRunner m_mushroomRunner;

    double m_lat = 0.0;
    double m_lon = 0.0;
    QString m_date;
    SurfaceWind m_surfaceWind;

    void onDownloadFinished(bool success, int exitCode);
    void onMushroomFinished(bool success, const QVector<MushroomMessage> &messages);
};
