#include "tilemath.h"
#include <cmath>
#include <algorithm>

namespace TileMath {

static constexpr double LAT_MAX = 85.05112878;

double clampLat(double lat) {
    return std::max(-LAT_MAX, std::min(LAT_MAX, lat));
}

int lonToTileX(double lon, int z) {
    return int(std::floor((lon + 180.0) / 360.0 * (1 << z)));
}

int latToTileY(double lat, int z) {
    double rad = clampLat(lat) * M_PI / 180.0;
    double n   = 1.0 - std::log(std::tan(rad) + 1.0 / std::cos(rad)) / M_PI;
    return int(std::floor(n / 2.0 * (1 << z)));
}

qint64 tileCount(double minLat, double minLon,
                 double maxLat, double maxLon,
                 int minZ,      int maxZ)
{
    qint64 total = 0;
    for (int z = minZ; z <= maxZ; ++z) {
        int x0 = lonToTileX(minLon, z);
        int x1 = lonToTileX(maxLon, z);
        int y0 = latToTileY(maxLat, z); // north → smaller y
        int y1 = latToTileY(minLat, z);
        if (x0 > x1) std::swap(x0, x1);
        if (y0 > y1) std::swap(y0, y1);
        // Clamp to valid range
        int maxIdx = (1 << z) - 1;
        x0 = std::max(0, std::min(maxIdx, x0));
        x1 = std::max(0, std::min(maxIdx, x1));
        y0 = std::max(0, std::min(maxIdx, y0));
        y1 = std::max(0, std::min(maxIdx, y1));
        total += qint64(x1 - x0 + 1) * qint64(y1 - y0 + 1);
    }
    return total;
}

} // namespace TileMath
