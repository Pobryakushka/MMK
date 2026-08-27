#include "GribMeteo11Pipeline.h"
#include "Meteo11ProfileBuilder.h"
#include "Meteo11Calculator.h"
#include "GribWindProfileAdapter.h"

namespace {
constexpr int kGfsCycleHours = 6; // GFS считается 4 раза в сутки: 00/06/12/18 UTC

// Консервативная оценка задержки публикации GFS 0.25° (f000) на NOMADS
// после времени цикла. Реально обычно ~3.5-4 ч, берём с запасом.
constexpr int kGfsPublishLatencyHours = 4;

// Ограничение на число шагов отката назад при выборе цикла — защита от
// зацикливания/ухода в глубокое прошлое при некорректных системных часах.
constexpr int kMaxCycleStepBack = 8; // 8 * 6ч = 48ч
}

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
    // GFS индексируется по UTC; dt обычно приходит в локальном времени
    // машины (QDateTime::currentDateTime() по всему проекту) — без этого
    // перевода здесь примерно в половине случаев суток выбирался бы не
    // тот 6-часовой цикл (а у полуночи — ещё и не та дата, см.
    // selectGfsCycle(), которая это уже учитывает).
    const int hour = dt.toUTC().time().hour();
    const int cycle = (hour / kGfsCycleHours) * kGfsCycleHours; // 0, 6, 12, 18
    return QString("%1").arg(cycle, 2, 10, QChar('0'));
}

void GribMeteo11Pipeline::selectGfsCycle(const QDateTime &sondingTime, QString &outDate, QString &outCycle)
{
    QDateTime cycleUtc = sondingTime.toUTC();
    const int hour = cycleUtc.time().hour();
    cycleUtc.setTime(QTime((hour / kGfsCycleHours) * kGfsCycleHours, 0, 0));

    // Если выбранный цикл ещё не мог быть опубликован на NOMADS к текущему
    // реальному моменту — откатываемся на предыдущий. Для архивных записей
    // (sondingTime сильно в прошлом) условие сразу ложно, откат не
    // происходит — задержка публикации актуальна только для "почти
    // текущих" запросов.
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    int guard = 0;
    while (guard < kMaxCycleStepBack
           && cycleUtc.addSecs(kGfsPublishLatencyHours * 3600) > nowUtc) {
        cycleUtc = cycleUtc.addSecs(-kGfsCycleHours * 3600);
        ++guard;
    }

    outDate  = cycleUtc.date().toString("yyyyMMdd");
    outCycle = QString("%1").arg(cycleUtc.time().hour(), 2, 10, QChar('0'));
}

void GribMeteo11Pipeline::run(double lat, double lon, const QDateTime &sondingTime,
                               double surfaceSpeedMs, double surfaceDirectionDeg)
{
    m_lat = lat;
    m_lon = lon;
    m_surfaceWind = GribWindProfileAdapter::surfaceWindFromSpeedDir(surfaceSpeedMs, surfaceDirectionDeg);

    QString date, runCycle;
    selectGfsCycle(sondingTime, date, runCycle);
    m_date = date;
    m_runCycle = runCycle;

    emit logLine(QStringLiteral("GRIB Метео-11: точка %1,%2, дата %3, цикл %4")
                     .arg(lat).arg(lon).arg(m_date, runCycle));

    m_downloadRunner.start(m_config.downloadScriptPath, m_date, runCycle, lat, lon);
}

QString GribMeteo11Pipeline::pointDataDir() const
{
    // Должно совпадать с тем, как grib.sh переопределяет FILES_DIR в
    // ветке --point: "${FILES_DIR}/${CURRENT_DATE}_${RUN}_pt${POINT_LAT}_${POINT_LONG}"
    return QStringLiteral("%1/%2_%3_pt%4_%5")
        .arg(m_config.dataDir, m_date, m_runCycle,
             GfsDownloadRunner::formatCoord(m_lat), GfsDownloadRunner::formatCoord(m_lon));
}

void GribMeteo11Pipeline::onDownloadFinished(bool success, int exitCode)
{
    if (!success) {
        emit finished(false, {}, QStringLiteral("Скачивание GRIB завершилось с ошибкой (код %1)").arg(exitCode));
        return;
    }

    emit logLine(QStringLiteral("GRIB скачан, запускаем Mushroom..."));
    m_mushroomRunner.start(m_config.mushroomExePath, pointDataDir(), m_lat, m_lon, m_date);
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
