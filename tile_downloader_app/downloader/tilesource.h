#pragma once
#include "core/tileid.h"
#include <QString>

// Describes a tile server: URL template + network identity.
// Kept separate from core/ so the core has zero network dependencies.
struct TileSource {
    QString urlTemplate = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
    // OSM tile usage policy requires a descriptive User-Agent with contact info.
    QString userAgent   = "TileDownloaderApp/1.0 (https://github.com/your-org/mmk)";

    QString buildUrl(const TileId &id) const {
        QString url = urlTemplate;
        url.replace("{z}", QString::number(id.z));
        url.replace("{x}", QString::number(id.x));
        url.replace("{y}", QString::number(id.y));
        return url;
    }

    bool isValid() const {
        return !urlTemplate.isEmpty()
            && urlTemplate.contains("{z}")
            && urlTemplate.contains("{x}")
            && urlTemplate.contains("{y}");
    }
};
