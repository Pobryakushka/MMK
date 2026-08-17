#include "GribMeteo11Pipeline.h"
#include "Meteo11ProfileBuilder.h"
#include "Meteo11Calculator.h"
#include "GribWindProfileAdapter.h"

GribMeteo11Pipeline::GribMeteo11Pipeline(QObject *parent) : QObject(parent)
{
    connect(&m_downloadRunner, &GfsDownloadRunner::logLine, this, &GribMeteo11Pipeline::logLine);
    connect(&m_downloadRunner, &GfsDownloadRunner::finished,
            this, &GribMeteo11Pipeline::onDownloadFinished);

    connect(&m_mushroomRunner, &MushroomRunner::logLine, this, &GribMeteo11Pipeline::logLine);
    connect(&m_mushroomRunner, &MushroomRunner::finished,
            this, &GribMeteo11Pipeline::onMushroomFinished);
}

QString GribMeteo11Pipeline::nearestGfsRunCycle(const QDateTime &dt)
{
    const int hour = dt.time().hour();
    const int cycle = (hour / 6) * 6; // 0, 6, 12, 18
    return QString("%1").arg(cycle, 2, 10, QChar('0'));
}

void GribMeteo11Pipeline::run(double lat, double lon, const QDateTime &sondingTime,
                               double surfaceSpeedMs, double surfaceDirectionDeg)
{
    m_lat = lat;
    m_lon = lon;
    m_date = sondingTime.date().toString("yyyyMMdd");
    m_surfaceWind = GribWindProfileAdapter::surfaceWindFromSpeedDir(surfaceSpeedMs, surfaceDirectionDeg);

    const QString runCycle = nearestGfsRunCycle(sondingTime);

    emit logLine(QStringLiteral("GRIB Метео-11: точка %1,%2, дата %3, цикл %4")
                     .arg(lat).arg(lon).arg(m_date, runCycle));

    m_downloadRunner.start(m_config.downloadScriptPath, m_date, runCycle, lat, lon);
}

void GribMeteo11Pipeline::onDownloadFinished(bool success, int exitCode)
{
    if (!success) {
        emit finished(false, {}, QStringLiteral("Скачивание GRIB завершилось с ошибкой (код %1)").arg(exitCode));
        return;
    }

    emit logLine(QStringLiteral("GRIB скачан, запускаем Mushroom..."));
    m_mushroomRunner.start(m_config.mushroomExePath, m_config.dataDir, m_lat, m_lon, m_date);
}

void GribMeteo11Pipeline::onMushroomFinished(bool success, const QVector<MushroomMessage> &messages)
{
    if (!success) {
        emit finished(false, {}, QStringLiteral("Mushroom завершилась с ошибкой"));
        return;
    }

    const auto buildResult = Meteo11ProfileBuilder::build(messages);
    if (!buildResult.ok) {
        emit finished(false, {}, buildResult.error);
        return;
    }

    const auto layerResults = Meteo11Calculator::compute(m_surfaceWind, buildResult.profile);
    const auto windProfile = GribWindProfileAdapter::toWindProfile(layerResults);

    emit logLine(QStringLiteral("GRIB Метео-11: рассчитано %1 слоёв").arg(windProfile.size()));
    emit finished(true, windProfile, QString());
}
