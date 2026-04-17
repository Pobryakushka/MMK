#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QPointer>

class QNetworkAccessManager;

// Minimal HTTP server serving map tiles from an MBTiles (SQLite) file.
//
// Online proxy mode (upstreamTemplate non-empty):
//   Cache miss → fetch from upstreamTemplate, write to MBTiles, serve.
//   upstreamTemplate uses Qt arg() syntax: %1=z, %2=x, %3=y.
//   Example: "https://a.tile.openstreetmap.org/%1/%2/%3.png"
//
// Offline mode (upstreamTemplate empty):
//   Cache miss → 404.
//
// TMS y-flip is applied automatically (MBTiles stores TMS y, OSM sends slippy y).
class LocalTileServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalTileServer(QObject *parent = nullptr);
    ~LocalTileServer() override;

    // Open (or create) an MBTiles file and start listening.
    // Returns false if the database or the TCP server cannot be opened.
    bool open(const QString &mbtilesPath,
              const QString &upstreamTemplate = QString());

    void close();

    quint16 port()       const { return m_port; }
    bool    isListening() const { return m_server && m_server->isListening(); }

    // URL template for Qt OSM provider JSON: "http://127.0.0.1:PORT/%z/%x/%y.png"
    QString tileUrlTemplate() const;

private slots:
    void onNewConnection();

private:
    void    handleSocket(QTcpSocket *socket, const QByteArray &req);
    QByteArray queryTile(int z, int x, int y);
    bool    writeTile(int z, int x, int y, const QByteArray &data);
    void    sendResponse(QTcpSocket *socket, int code, const QByteArray &data);

    QTcpServer           *m_server   = nullptr;
    QNetworkAccessManager *m_nam     = nullptr;
    QSqlDatabase          m_db;
    QString               m_upstreamTemplate;
    quint16               m_port     = 0;
    int                   m_connSeq  = 0;
};
