#pragma once
#include <QString>

// Одна строка результата, которую выдаёт Mushroom для одного
// метеопараметра на одном уровне (соответствует struct Message в
// Mushroom.hpp, но не зависит от eccodes/armadillo — сюда попадают
// только уже посчитанные значения из текстового вывода программы).
struct MushroomMessage {
    QString fileName;
    QString centre;
    QString parameterName;   // "Temperature", "U_ComponentOfWind" и т.п.
    double parameterValue = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double distance = -1.0;
    long level = 0;
    long validityDate = 0;
    long validityTime = 0;
    long dataDate = 0;
    long dataTime = 0;
};
