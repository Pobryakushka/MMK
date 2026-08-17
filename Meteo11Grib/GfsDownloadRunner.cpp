#include "GfsDownloadRunner.h"
#include <QFileInfo>

GfsDownloadRunner::GfsDownloadRunner(QObject *parent) : QObject(parent)
{
    connect(&m_runner, &ProcessRunner::outputLine,
            this, &GfsDownloadRunner::logLine);

    connect(&m_runner, &ProcessRunner::failedToStart, this,
            [this](const QString &reason) {
                emit logLine(QStringLiteral("Не удалось запустить скрипт: %1").arg(reason));
                emit finished(false, -1);
            });

    connect(&m_runner, &ProcessRunner::finished, this,
            [this](int exitCode, bool crashed) {
                emit finished(!crashed && exitCode == 0, exitCode);
            });
}

void GfsDownloadRunner::start(const QString &scriptPath, const QString &date,
                               const QString &run, double lat, double lon)
{
    // Формат точки соответствует ожидаемому в gfs_download.sh: LAT:LONG
    const QString point = QStringLiteral("%1:%2")
        .arg(lat, 0, 'f', 4)
        .arg(lon, 0, 'f', 4);

    // START=0 END=0 — нам всегда нужен только текущий момент (f000),
    // прогноз на будущее для этой задачи не требуется.
    const QStringList args = {
        date, run, "0", "0", "--point", point
    };

    const QString workDir = QFileInfo(scriptPath).absolutePath();
    m_runner.start(QStringLiteral("bash"),
                   QStringList{scriptPath} + args,
                   workDir);
}
