#pragma once
#include <QObject>
#include <QVector>
#include <QDateTime>
#include "core/grib/GfsDownloadRunner.h"
#include "core/grib/MushroomRunner.h"
#include "core/grib/GribConfig.h"
#include "core/grib/Meteo11Types.h"
#include "devices/ams/amsprotocol.h" // WindProfileData

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
    // dt может быть в любом timeSpec (переводится в UTC внутри — циклы GFS
    // всегда в UTC, а sondingTime в остальной программе хранится в
    // локальном времени машины, см. QDateTime::currentDateTime() в
    // amshandler.cpp). Публичный static — пригодится и для UI (показать
    // пользователю, какой цикл будет запрошен, до нажатия кнопки).
    // Не учитывает задержку публикации на NOMADS — см. selectGfsCycle().
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
    QString m_runCycle;
    SurfaceWind m_surfaceWind;

    void onDownloadFinished(bool success, int exitCode);
    void onMushroomFinished(bool success, const QVector<MushroomMessage> &messages);

    // Ближайший (не позже sondingTime, в UTC) цикл GFS, отступая на
    // предыдущий цикл, если самый свежий ещё не мог быть опубликован на
    // NOMADS (задержка публикации ~4 ч после времени цикла, см. .cpp).
    // В отличие от nearestGfsRunCycle() возвращает сразу и дату, и цикл,
    // так как при откате назад или при переводе в UTC у полуночи может
    // измениться и календарная дата, а не только час.
    static void selectGfsCycle(const QDateTime &sondingTime, QString &outDate, QString &outCycle);

    // Каталог, куда grib.sh реально кладёт скачанный файл для текущего
    // запроса (m_date/m_runCycle/m_lat/m_lon) — см. FILES_DIR в
    // grib.sh (--point ветка). Должен совпадать с ним посимвольно.
    QString pointDataDir() const;
};
