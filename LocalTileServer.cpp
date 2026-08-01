#include "LocalTileServer.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QHostAddress>
#include <QDebug>

LocalTileServer::LocalTileServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &LocalTileServer::onNewConnection);
}

LocalTileServer::~LocalTileServer()
{
    close();
}

// ── Public API ────────────────────────────────────────────────────────────────

bool LocalTileServer::start()
{
    if (m_server->isListening()) return true;

    if (!m_server->listen(QHostAddress::LocalHost)) {
        qWarning() << "LocalTileServer: cannot bind:" << m_server->errorString();
        return false;
    }
    m_port = m_server->serverPort();
    qDebug() << "LocalTileServer: started on port" << m_port;
    return true;
}

bool LocalTileServer::switchTo(const QString &mbtilesPath, const QString &upstreamTemplate)
{
    closeDb();
    m_upstreamTemplate = upstreamTemplate;

    if (!openDb(mbtilesPath)) {
        qWarning() << "LocalTileServer: cannot open DB:" << mbtilesPath;
        return false;
    }

    if (!upstreamTemplate.isEmpty() && !m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }

    qDebug() << "LocalTileServer: switchTo" << mbtilesPath
             << "| proxy:" << (!upstreamTemplate.isEmpty() ? "yes" : "no");
    return true;
}

void LocalTileServer::close()
{
    closeDb();
    if (m_server && m_server->isListening())
        m_server->close();
    m_port = 0;
}

QString LocalTileServer::tileUrlTemplate() const
{
    return QString("http://127.0.0.1:%1/%z/%x/%y.png").arg(m_port);
}

// ── Private: DB ───────────────────────────────────────────────────────────────

bool LocalTileServer::openDb(const QString &path)
{
    QString connName = QString("LTS_%1_%2")
        .arg(reinterpret_cast<quintptr>(this)).arg(++m_connSeq);
    m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        QSqlDatabase::removeDatabase(connName);
        return false;
    }
    QSqlQuery q(m_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("CREATE TABLE IF NOT EXISTS metadata "
           "(name TEXT NOT NULL, value TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS tiles "
           "(zoom_level INTEGER, tile_column INTEGER, tile_row INTEGER, tile_data BLOB, "
           "PRIMARY KEY (zoom_level, tile_column, tile_row))");
    return true;
}

void LocalTileServer::closeDb()
{
    if (m_db.isOpen()) {
        QString c = m_db.connectionName();
        m_db.close();
        if (!c.isEmpty()) QSqlDatabase::removeDatabase(c);
    }
}

// ── Private: network ─────────────────────────────────────────────────────────

void LocalTileServer::onNewConnection()
{
    QTcpSocket *socket = m_server->nextPendingConnection();
    // One-shot: full HTTP request always fits in one loopback TCP segment
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);
        handleSocket(socket, socket->readAll());
    });
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void LocalTileServer::handleSocket(QTcpSocket *socket, const QByteArray &req)
{
    // Parse "GET /Z/X/Y.png HTTP/1.1" – ignore extension
    static QRegularExpression re("GET /(\\d+)/(\\d+)/(\\d+)");
    auto m = re.match(req);
    if (!m.hasMatch()) {
        sendResponse(socket, 400, {});
        socket->disconnectFromHost();
        return;
    }

    int z = m.captured(1).toInt();
    int x = m.captured(2).toInt();
    int y = m.captured(3).toInt();   // OSM/slippy y (0 = north)

    QByteArray tile = queryTile(z, x, y);
    if (!tile.isEmpty()) {
        qDebug() << "LocalTileServer: HIT  z" << z << "x" << x << "y" << y
                 << "yTms" << ((1<<z)-1-y) << "size" << tile.size();
        sendResponse(socket, 200, tile);
        socket->disconnectFromHost();
        return;
    }

    if (m_upstreamTemplate.isEmpty()) {
        qDebug() << "LocalTileServer: MISS (offline) z" << z << "x" << x << "y" << y
                 << "yTms" << ((1<<z)-1-y);
        sendResponse(socket, 404, {});
        socket->disconnectFromHost();
        return;
    }

    // Proxy to upstream
    QUrl url(m_upstreamTemplate.arg(z).arg(x).arg(y));
    qDebug() << "LocalTileServer: PROXY z" << z << "x" << x << "y" << y << "->" << url;
    QNetworkRequest netReq(url);
    netReq.setHeader(QNetworkRequest::UserAgentHeader, "MMK/1.0");
    netReq.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QPointer<QTcpSocket> safeSocket(socket);
    auto *reply = m_nam->get(netReq);

    connect(reply, &QNetworkReply::finished, this, [this, safeSocket, reply, z, x, y]() {
        reply->deleteLater();
        QByteArray data = reply->readAll();
        if (reply->error() == QNetworkReply::NoError && !data.isEmpty()) {
            writeTile(z, x, y, data);
            if (safeSocket) {
                sendResponse(safeSocket, 200, data);
                safeSocket->disconnectFromHost();
            }
        } else {
            qDebug() << "LocalTileServer: upstream error z" << z << "x" << x << "y" << y
                     << reply->errorString();
            if (safeSocket) {
                sendResponse(safeSocket, 502, {});
                safeSocket->disconnectFromHost();
            }
        }
    });
}

QByteArray LocalTileServer::queryTile(int z, int x, int y)
{
    if (!m_db.isOpen()) return {};
    // MBTiles stores TMS y (y=0 at south); slippy y (y=0 at north) must be flipped
    int yTms = (1 << z) - 1 - y;
    QSqlQuery q(m_db);
    q.prepare("SELECT tile_data FROM tiles "
              "WHERE zoom_level=? AND tile_column=? AND tile_row=?");
    q.addBindValue(z);
    q.addBindValue(x);
    q.addBindValue(yTms);
    if (q.exec() && q.next())
        return q.value(0).toByteArray();
    return {};
}

bool LocalTileServer::writeTile(int z, int x, int y, const QByteArray &data)
{
    if (!m_db.isOpen()) return false;
    int yTms = (1 << z) - 1 - y;
    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO tiles "
              "(zoom_level, tile_column, tile_row, tile_data) VALUES (?,?,?,?)");
    q.addBindValue(z);
    q.addBindValue(x);
    q.addBindValue(yTms);
    q.addBindValue(data);
    return q.exec();
}

void LocalTileServer::sendResponse(QTcpSocket *socket, int code, const QByteArray &data)
{
    const char *status = (code == 200) ? "200 OK"
                       : (code == 404) ? "404 Not Found"
                       : (code == 400) ? "400 Bad Request"
                                       : "502 Bad Gateway";
    QByteArray header =
        QByteArrayLiteral("HTTP/1.1 ") + status +
        "\r\nContent-Type: image/png\r\nContent-Length: " +
        QByteArray::number(data.size()) +
        "\r\nConnection: close\r\n\r\n";
    socket->write(header);
    if (!data.isEmpty())
        socket->write(data);
}
