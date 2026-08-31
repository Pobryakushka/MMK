#pragma once
#include <QObject>
#include "core/grib/ProcessRunner.h"

// Оборачивает запуск gfs_download.sh для конкретной точки (LAT:LONG)
// через режим --point (см. доработку скрипта). Всё знание о том, "как
// собрать аргументы командной строки скрипта", сосредоточено здесь —
// при интеграции в основной проект (или при изменении контракта
// скрипта) достаточно поменять только этот класс.
class GfsDownloadRunner : public QObject {
    Q_OBJECT
public:
    explicit GfsDownloadRunner(QObject *parent = nullptr);

    void start(const QString &scriptPath, const QString &date,
               const QString &run, double lat, double lon);

    // Единый формат координаты (4 знака после запятой) для точки —
    // используется и здесь при формировании аргумента --point, и в
    // GribMeteo11Pipeline::pointDataDir() при вычислении пути каталога,
    // куда grib.sh реально положит скачанный файл. Важно, чтобы оба
    // места форматировали координату одинаково, иначе Mushroom будет
    // запущен не на том каталоге, куда скрипт что-то скачал.
    static QString formatCoord(double value);

signals:
    void logLine(const QString &line);
    void finished(bool success, int exitCode);

private:
    ProcessRunner m_runner;
};
