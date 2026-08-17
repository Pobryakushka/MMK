#pragma once
#include <QString>

// Пути и параметры GRIB-пайплайна. Вынесены отдельно, чтобы потом
// легко подключить к реальной системе настроек MMK (сейчас — просто
// значения по умолчанию с возможностью переопределить перед run()).
struct GribConfig {
    QString downloadScriptPath = "/home/otdel412/Projects/MMK/GRIB/SocketSpecial2G/grib.sh";
    QString mushroomExePath = "/home/otdel412/Projects/MMK/GRIB/Mushroom/mushroom/build/Desktop-Release/Main";
    QString dataDir = "/home/otdel412/Projects/MMK/GRIB/SocketSpecial2G/Send/Files";

    double pointWindowDeg = 5.0; // должно совпадать с POINT_WINDOW в gfs_download.sh
};
