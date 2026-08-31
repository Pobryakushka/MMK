#include "core/grib/ProcessRunner.h"

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
            [this](QProcess::ProcessError error) {
                // failedToStart() должен означать ровно то, что говорит его имя:
                // процесс не удалось запустить вообще (нет файла, нет прав и
                // т.п.) — единственный случай, когда QProcess::finished()
                // после этого не придёт вовсе, и сообщить о провале должны
                // мы сами. Раньше здесь проверялось state()==NotRunning —
                // ненадёжный признак: он не документирован Qt как гарантия
                // именно для FailedToStart, и при обычном крэше уже
                // запущенного процесса эмпирически (Qt 5.15.8/Linux) state()
                // в момент errorOccurred(Crashed) всё ещё Running, так что
                // на этой платформе сейчас дубля нет — но полагаться на
                // недокументированное поведение (тем более что проект
                // собирается и под Windows, где тайминг QProcess может
                // отличаться) не стоит. Проверяем сам код ошибки явно.
                if (error == QProcess::FailedToStart)
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
