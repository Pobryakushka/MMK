#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QPointer>

class QNetworkAccessManager;

// Local HTTP server serving map tiles from an MBTiles (SQLite) file.
//
// Design: the server starts ONCE and keeps the same TCP port for the lifetime
// of the application. Switching between different MBTiles files or between
// online/offline modes is done by calling switchTo() — the port and provider
// JSON URL never change, so the Qt map plugin needs no re-initialisation.
//
// Online proxy mode (non-empty upstreamTemplate):
//   Cache miss → fetch from OSM → write to MBTiles → serve.
//   upstreamTemplate uses Qt arg() placeholders: %1=z, %2=x, %3=y.
//
// Offline mode (empty upstreamTemplate):
//   Cache miss → 404.  No network access at all.
//
// TMS y-flip is applied automatically (MBTiles tile_row = TMS, Qt sends OSM).
class LocalTileServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalTileServer(QObject *parent = nullptr);
    ~LocalTileServer() override;

    // Start the TCP server (call once). Returns false if the port cannot bind.
    bool start();

    // Switch the active database without restarting the server.
    // mbtilesPath will be created if it does not exist.
    // upstreamTemplate empty = offline only; non-empty = online proxy mode.
    bool switchTo(const QString &mbtilesPath,
                  const QString &upstreamTemplate = QString());

    void close();

    quint16 port()        const { return m_port; }
    bool    isListening() const { return m_server && m_server->isListening(); }

    // URL template for Qt OSM provider JSON: "http://127.0.0.1:PORT/%z/%x/%y.png"
    QString tileUrlTemplate() const;

private slots:
    void onNewConnection();

private:
    void       handleSocket(QTcpSocket *socket, const QByteArray &req);
    QByteArray queryTile(int z, int x, int y);
    bool       writeTile(int z, int x, int y, const QByteArray &data);
    void       sendResponse(QTcpSocket *socket, int code, const QByteArray &data);
    bool       openDb(const QString &path);
    void       closeDb();

    QTcpServer            *m_server   = nullptr;
    QNetworkAccessManager *m_nam      = nullptr;
    QSqlDatabase           m_db;
    QString                m_upstreamTemplate;
    quint16                m_port     = 0;
    int                    m_connSeq  = 0;
};
