#pragma once
#include <QtGlobal>

struct TileId {
    int z = 0;
    int x = 0;
    int y = 0;

    bool operator==(const TileId &o) const noexcept {
        return z == o.z && x == o.x && y == o.y;
    }
    bool operator!=(const TileId &o) const noexcept { return !(*this == o); }
};

// Collision-free hash for valid tile coords (z≤19, x/y < 2^19)
inline uint qHash(const TileId &id, uint seed = 0) noexcept {
    // Pack into 43 bits: 5 bits z | 19 bits x | 19 bits y
    quint64 key = (quint64(id.z & 0x1F) << 38)
                | (quint64(id.x & 0x7FFFF) << 19)
                | (quint64(id.y & 0x7FFFF));
    return ::qHash(key, seed);
}
