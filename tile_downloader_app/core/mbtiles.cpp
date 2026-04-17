#include "mbtiles.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

int MBTilesWriter::s_serial = 0;

MBTilesWriter::MBTilesWriter()
{
    m_connectionName = QString("mbtiles_%1").arg(++s_serial);
}

MBTilesWriter::~MBTilesWriter()
{
    close();
}

bool MBTilesWriter::open(const QString &path)
{
    close();
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    // Performance pragmas for bulk inserts
    execSql("PRAGMA journal_mode=WAL");
    execSql("PRAGMA synchronous=NORMAL");
    execSql("PRAGMA cache_size=4000");
    return createSchema();
}

void MBTilesWriter::close()
{
    if (m_db.isOpen()) {
        if (m_inTransaction) {
            m_db.commit();
            m_inTransaction = false;
        }
        m_db.close();
    }
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool MBTilesWriter::isOpen() const
{
    return m_db.isOpen();
}

bool MBTilesWriter::createSchema()
{
    return execSql(
        "CREATE TABLE IF NOT EXISTS tiles ("
        "  zoom_level  INTEGER NOT NULL,"
        "  tile_column INTEGER NOT NULL,"
        "  tile_row    INTEGER NOT NULL,"
        "  tile_data   BLOB    NOT NULL,"
        "  PRIMARY KEY (zoom_level, tile_column, tile_row)"
        ")"
    ) && execSql(
        "CREATE TABLE IF NOT EXISTS metadata ("
        "  name  TEXT PRIMARY KEY,"
        "  value TEXT"
        ")"
    );
}

QSet<TileId> MBTilesWriter::loadExistingTiles()
{
    QSet<TileId> result;
    if (!m_db.isOpen()) return result;

    QSqlQuery q(m_db);
    if (!q.exec("SELECT zoom_level, tile_column, tile_row FROM tiles")) {
        m_lastError = q.lastError().text();
        return result;
    }
    while (q.next()) {
        int z    = q.value(0).toInt();
        int x    = q.value(1).toInt();
        int yTms = q.value(2).toInt();
        // Convert TMS y back to OSM y
        int y = (1 << z) - 1 - yTms;
        result.insert(TileId{z, x, y});
    }
    return result;
}

void MBTilesWriter::setMetadata(const QString &name, const QString &value)
{
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO metadata (name, value) VALUES (?, ?)");
    q.addBindValue(name);
    q.addBindValue(value);
    q.exec();
}

bool MBTilesWriter::beginBatch()
{
    if (!m_db.isOpen() || m_inTransaction) return false;
    m_inTransaction = m_db.transaction();
    return m_inTransaction;
}

bool MBTilesWriter::commitBatch()
{
    if (!m_inTransaction) return false;
    bool ok = m_db.commit();
    m_inTransaction = false;
    if (!ok) m_lastError = m_db.lastError().text();
    return ok;
}

bool MBTilesWriter::rollbackBatch()
{
    if (!m_inTransaction) return false;
    bool ok = m_db.rollback();
    m_inTransaction = false;
    return ok;
}

bool MBTilesWriter::insertTile(const TileId &id, const QByteArray &data)
{
    if (!m_db.isOpen()) return false;
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR REPLACE INTO tiles "
        "(zoom_level, tile_column, tile_row, tile_data) "
        "VALUES (?, ?, ?, ?)"
    );
    q.addBindValue(id.z);
    q.addBindValue(id.x);
    q.addBindValue(toTmsY(id.y, id.z)); // OSM→TMS y flip
    q.addBindValue(data);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

qint64 MBTilesWriter::tileCount()
{
    if (!m_db.isOpen()) return 0;
    QSqlQuery q(m_db);
    q.exec("SELECT COUNT(*) FROM tiles");
    if (q.next()) return q.value(0).toLongLong();
    return 0;
}

bool MBTilesWriter::execSql(const QString &sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}
