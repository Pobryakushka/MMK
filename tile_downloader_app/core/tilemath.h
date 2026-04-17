#pragma once
#include <QtGlobal>

// Standard OSM/Slippy-Map tile coordinate math.
// No URL logic here — tile sources live in downloader/tilesource.h.
namespace TileMath {

int lonToTileX(double lon, int z);
int latToTileY(double lat, int z);  // OSM convention: y=0 at top (north)

// Total tile count for a bounding box across a zoom range.
// Safe to call with large ranges — purely arithmetic, no allocation.
qint64 tileCount(double minLat, double minLon,
                 double maxLat, double maxLon,
                 int minZ,      int maxZ);

// Clamp lat to Mercator limits (~85.05°)
double clampLat(double lat);

} // namespace TileMath
