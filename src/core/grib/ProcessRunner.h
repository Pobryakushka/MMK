#pragma once
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// Тонкая обёртка над QProcess. Не содержит доменной логики — умеет
// только запустить произвольную программу, стримить её вывод построчно
// и сообщить о завершении. Используется как общий строительный блок для
// GfsDownloadRunner и MushroomRunner, а также для любых других внешних
// процессов, которые понадобятся при интеграции в основной проект.
class ProcessRunner : public QObject {
    Q_OBJECT
public:
    explicit ProcessRunner(QObject *parent = nullptr);

    // program          - путь к исполняемому файлу/интерпретатору
    // arguments        - аргументы командной строки
    // workingDirectory - рабочая директория процесса (можно оставить пустой)
    void start(const QString &program, const QStringList &arguments,
               const QString &workingDirectory = QString());

    bool isRunning() const;

signals:
    void outputLine(const QString &line);       // очередная строка stdout/stderr
    void finished(int exitCode, bool crashed);
    void failedToStart(const QString &reason);

private:
    QProcess m_process;
    QString m_pendingBuffer;

    void handleReadyRead();
};
