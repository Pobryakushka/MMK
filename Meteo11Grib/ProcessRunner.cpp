#include "ProcessRunner.h"

ProcessRunner::ProcessRunner(QObject *parent) : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);

    connect(&m_process, &QProcess::readyRead,
            this, &ProcessRunner::handleReadyRead);

    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus status) {
                // Дочитываем всё, что осталось в буфере без завершающего '\n'
                if (!m_pendingBuffer.trimmed().isEmpty())
                    emit outputLine(m_pendingBuffer.trimmed());
                m_pendingBuffer.clear();

                emit finished(exitCode, status == QProcess::CrashExit);
            });

    connect(&m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
                if (m_process.state() == QProcess::NotRunning)
                    emit failedToStart(m_process.errorString());
            });
}

void ProcessRunner::start(const QString &program, const QStringList &arguments,
                           const QString &workingDirectory)
{
    if (!workingDirectory.isEmpty())
        m_process.setWorkingDirectory(workingDirectory);

    m_pendingBuffer.clear();
    m_process.start(program, arguments);
}

bool ProcessRunner::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void ProcessRunner::handleReadyRead()
{
    m_pendingBuffer += QString::fromUtf8(m_process.readAll());

    int newlineIndex;
    while ((newlineIndex = m_pendingBuffer.indexOf('\n')) != -1) {
        const QString line = m_pendingBuffer.left(newlineIndex).trimmed();
        m_pendingBuffer.remove(0, newlineIndex + 1);
        if (!line.isEmpty())
            emit outputLine(line);
    }
}
