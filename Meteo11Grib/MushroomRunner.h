#pragma once
#include <QObject>
#include <QVector>
#include "ProcessRunner.h"
#include "MushroomMessage.h"

// Оборачивает запуск программы Mushroom (Main) для конкретной точки
// и парсинг её вывода в список MushroomMessage. Не модифицирует и не
// зависит от исходников самой Mushroom — работает исключительно через
// её командную строку и стандартный вывод.
class MushroomRunner : public QObject {
    Q_OBJECT
public:
    explicit MushroomRunner(QObject *parent = nullptr);

    void start(const QString &exePath, const QString &dataDir,
               double lat, double lon, const QString &date);

signals:
    void logLine(const QString &line);
    void finished(bool success, const QVector<MushroomMessage> &messages);

private:
    ProcessRunner m_runner;
    QString m_rawOutput;
};
