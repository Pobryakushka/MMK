#include "GribWindProfileAdapter.h"
#include <cmath>

QVector<WindProfileData> GribWindProfileAdapter::toWindProfile(const QVector<Meteo11LayerResult> &results)
{
    QVector<WindProfileData> profile;
    profile.reserve(results.size());

    for (const auto &r : results) {
        WindProfileData wp;
        wp.height = static_cast<float>(r.layerTopM);
        wp.windSpeed = static_cast<float>(r.speedMs);
        wp.windDirection = static_cast<float>(r.directionDeg);
        wp.isValid = r.sufficientData;
        profile.push_back(wp);
    }

    return profile;
}

SurfaceWind GribWindProfileAdapter::surfaceWindFromSpeedDir(double speedMs, double directionDeg)
{
    // Точная инверсия формулы (6): V=sqrt(vz^2+vm^2), theta=asin(-vz/V) (vm<=0)
    // или theta=180-asin(-vz/V) (vm>0). Обращая:
    //   vz = -V * sin(theta)
    //   vm = -V * cos(theta)
    const double rad = directionDeg * M_PI / 180.0;
    SurfaceWind s;
    s.vz = -speedMs * std::sin(rad);
    s.vm = -speedMs * std::cos(rad);
    return s;
}
