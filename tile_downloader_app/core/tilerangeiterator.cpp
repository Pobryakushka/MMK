#include "tilerangeiterator.h"
#include "tilemath.h"
#include <algorithm>

TileRangeIterator::TileRangeIterator(double minLat, double minLon,
                                     double maxLat, double maxLon,
                                     int minZ,      int maxZ)
    : m_minLat(minLat), m_minLon(minLon)
    , m_maxLat(maxLat), m_maxLon(maxLon)
    , m_minZ(minZ),     m_maxZ(maxZ)
    , m_total(0), m_zIdx(0), m_curX(0), m_curY(0)
{
    buildSlices();
}

void TileRangeIterator::buildSlices()
{
    m_slices.clear();
    m_total = 0;

    for (int z = m_minZ; z <= m_maxZ; ++z) {
        int x0 = TileMath::lonToTileX(m_minLon, z);
        int x1 = TileMath::lonToTileX(m_maxLon, z);
        int y0 = TileMath::latToTileY(m_maxLat, z); // north → smaller y
        int y1 = TileMath::latToTileY(m_minLat, z);
        if (x0 > x1) std::swap(x0, x1);
        if (y0 > y1) std::swap(y0, y1);
        int maxIdx = (1 << z) - 1;
        x0 = std::max(0, std::min(maxIdx, x0));
        x1 = std::max(0, std::min(maxIdx, x1));
        y0 = std::max(0, std::min(maxIdx, y0));
        y1 = std::max(0, std::min(maxIdx, y1));
        m_slices.append({z, x0, x1, y0, y1});
        m_total += qint64(x1 - x0 + 1) * qint64(y1 - y0 + 1);
    }

    reset();
}

void TileRangeIterator::reset()
{
    m_zIdx = 0;
    if (!m_slices.isEmpty()) {
        m_curX = m_slices[0].x0;
        m_curY = m_slices[0].y0;
    }
}

bool TileRangeIterator::hasNext() const
{
    return m_zIdx < m_slices.size();
}

TileId TileRangeIterator::next()
{
    const ZoomSlice &s = m_slices[m_zIdx];
    TileId id{s.z, m_curX, m_curY};

    // Advance: y inner loop, x outer loop
    ++m_curY;
    if (m_curY > s.y1) {
        m_curY = s.y0;
        ++m_curX;
        if (m_curX > s.x1) {
            ++m_zIdx;
            if (m_zIdx < m_slices.size()) {
                m_curX = m_slices[m_zIdx].x0;
                m_curY = m_slices[m_zIdx].y0;
            }
        }
    }
    return id;
}
