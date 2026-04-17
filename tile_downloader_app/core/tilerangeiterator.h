#pragma once
#include "tileid.h"
#include <QtGlobal>

// Lazy iterator over all tiles in a bounding box across a zoom range.
// Generates TileIds on demand — no large QVector allocation.
// Iterates: for each z, for each x, for each y.
class TileRangeIterator {
public:
    TileRangeIterator(double minLat, double minLon,
                      double maxLat, double maxLon,
                      int minZ,      int maxZ);

    bool hasNext() const;
    TileId next();

    qint64 totalCount() const { return m_total; }

    // Reset to beginning (for restart without re-allocating)
    void reset();

private:
    struct ZoomSlice {
        int z, x0, x1, y0, y1;
    };

    double m_minLat, m_minLon, m_maxLat, m_maxLon;
    int    m_minZ,   m_maxZ;
    qint64 m_total;

    // Mutable iteration state
    int m_zIdx;             // index into m_slices
    int m_curX, m_curY;
    QVector<ZoomSlice> m_slices;

    void buildSlices();
};
