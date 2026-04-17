#pragma once
#include "tileid.h"
#include <QString>
#include <QSet>
#include <QSqlDatabase>

// MBTiles writer backed by SQLite.
// All methods must be called from ONE thread (the thread that called open()).
// Uses batched transactions: call beginBatch() / commitBatch() around
// groups of insertTile() calls for best performance.
//
// MBTiles y-convention: TMS (y=0 at south). OSM tiles use XYZ (y=0 at north).
// Conversion is done automatically: y_tms = (1<<z) - 1 - y_osm.
class MBTilesWriter {
public:
    MBTilesWriter();
    ~MBTilesWriter();

    // Open (or create) an MBTiles file.
    // Returns false and sets lastError() on failure.
    bool open(const QString &path);
    void close();
    bool isOpen() const;

    QString lastError() const { return m_lastError; }

    // Bulk-load existing tile coordinates into a QSet for O(1) resume checks.
    // Call once after open(), before starting download.
    QSet<TileId> loadExistingTiles();

    // Write metadata (name/value pairs per MBTiles spec).
    void setMetadata(const QString &name, const QString &value);

    // Transaction helpers — wrap groups of insertTile() for performance.
    bool beginBatch();
    bool commitBatch();
    bool rollbackBatch();

    // Insert one tile. Silently replaces existing tile with same coordinates.
    // Must be called inside a begin/commit batch for acceptable performance.
    bool insertTile(const TileId &id, const QByteArray &data);

    // How many tiles are stored
    qint64 tileCount();

private:
    QSqlDatabase m_db;
    QString      m_connectionName;
    QString      m_lastError;
    bool         m_inTransaction = false;

    static int   s_serial; // unique connection name per instance

    bool execSql(const QString &sql);
    bool createSchema();

    // MBTiles uses TMS y (origin south), OSM uses XYZ y (origin north)
    static int toTmsY(int y, int z) { return (1 << z) - 1 - y; }
};
