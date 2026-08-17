#include "MushroomRunner.h"
#include "MushroomResultParser.h"

MushroomRunner::MushroomRunner(QObject *parent) : QObject(parent)
{
    connect(&m_runner, &ProcessRunner::outputLine, this,
            [this](const QString &line) {
                m_rawOutput += line + QLatin1Char('\n');
                emit logLine(line);
            });

    connect(&m_runner, &ProcessRunner::failedToStart, this,
            [this](const QString &reason) {
                emit logLine(QStringLiteral("Не удалось запустить Mushroom: %1").arg(reason));
                emit finished(false, {});
            });

    connect(&m_runner, &ProcessRunner::finished, this,
            [this](int exitCode, bool crashed) {
                const auto messages = MushroomResultParser::parse(m_rawOutput);
                emit finished(!crashed && exitCode == 0, messages);
            });
}

void MushroomRunner::start(const QString &exePath, const QString &dataDir,
                            double lat, double lon, const QString &date)
{
    m_rawOutput.clear();

    // Аргументы соответствуют сигнатуре main() в Mushroom:
    // <папка> <LAT> <LONG> [дата]
    const QStringList args = {
        dataDir,
        QString::number(lat, 'f', 4),
        QString::number(lon, 'f', 4),
        date
    };

    m_runner.start(exePath, args);
}
