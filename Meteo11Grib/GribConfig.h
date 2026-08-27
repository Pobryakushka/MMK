#pragma once
#include <QString>

// Пути и параметры GRIB-пайплайна. Вынесены отдельно, чтобы потом
// легко подключить к реальной системе настроек MMK (сейчас — просто
// значения по умолчанию с возможностью переопределить перед run()).
struct GribConfig {
    QString downloadScriptPath = "/home/otdel412/Projects/MMK/GRIB/SocketSpecial2G/grib.sh";
    QString mushroomExePath = "/home/otdel412/Projects/MMK/GRIB/Mushroom/Desktop-Release/Main";

    // Базовый каталог для скачанных файлов. Реальный файл для конкретного
    // запроса кладётся в подкаталог dataDir/ДАТА_ЦИКЛ_ptШИРОТА_ДОЛГОТА (см.
    // GribMeteo11Pipeline::pointDataDir() и ветку --point в grib.sh) —
    // так разные даты/точки не путают друг друга в общем каталоге.
    QString dataDir = "/home/otdel412/Projects/MMK/GRIB/SocketSpecial2G/Send/Files";

    double pointWindowDeg = 2.0; // должно совпадать с POINT_WINDOW в grib.sh
};
