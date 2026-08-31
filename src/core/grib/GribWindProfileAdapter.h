#pragma once
#include <QVector>
#include "core/grib/Meteo11Types.h"
#include "devices/ams/amsprotocol.h" // WindProfileData

// Мост между расчётным движком (работает в терминах зональной/
// меридиональной составляющих ветра, м/с) и остальной программой MMK
// (работает в терминах "скорость + направление", WindProfileData).
class GribWindProfileAdapter {
public:
    // Meteo11LayerResult[] (12 слоёв, уже на стандартных высотах) ->
    // WindProfileData[], пригодный для передачи в уже существующий
    // MeasurementResults::buildMeteo11(). Слои с sufficientData=false
    // получают isValid=false — buildMeteo11() сам остановится на первом
    // таком слое и дальше пометит выше как недостижимые, что и нужно.
    static QVector<WindProfileData> toWindProfile(const QVector<Meteo11LayerResult> &results);

    // Обратное преобразование: скорость (м/с) + метеорологическое
    // направление (градусы, "откуда дует") -> SurfaceWind (зональная/
    // меридиональная составляющие). Нужно, чтобы использовать уже
    // имеющиеся в системе показания приземного датчика ветра
    // (m_currentWindSpeedSurface/m_currentWindDirSurface) как вход для
    // Meteo11Calculator — формула является точной инверсией формулы (6)
    // из методики (см. Meteo11Calculator.cpp).
    static SurfaceWind surfaceWindFromSpeedDir(double speedMs, double directionDeg);
};
