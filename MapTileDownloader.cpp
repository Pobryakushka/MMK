#include "MapTileDownloader.h"
#include <QNetworkRequest>
#include <QFile>
#include <QDebug>
#include <QtMath>

MapTileDownloader::MapTileDownloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_throttleTimer(new QTimer(this))
{
    m_throttleTimer->setInterval(550); // ~1.8 req/s — немного медленнее лимита OSM
    m_throttleTimer->setSingleShot(false);
    connect(m_throttleTimer, &QTimer::timeout, this, &MapTileDownloader::processQueue);
    connect(m_nam, &QNetworkAccessManager::finished, this, &MapTileDownloader::onReplyFinished);
}

int MapTileDownloader::lonToX(double lon, int z)
{
    return (int)qFloor((lon + 180.0) / 360.0 * (1 << z));
}

int MapTileDownloader::latToY(double lat, int z)
{
    double latRad = qDegreesToRadians(lat);
    return (int)qFloor((1.0 - qLn(qTan(latRad) + 1.0 / qCos(latRad)) / M_PI) / 2.0 * (1 << z));
}

void MapTileDownloader::download(const QString &outputDir,
                                  double north, double south,
                                  double west,  double east,
                                  int minZoom,  int maxZoom)
{
    if (m_active) return;

    m_outputDir   = outputDir;
    m_total       = 0;
    m_downloaded  = 0;
    m_failed      = 0;
    m_inFlight    = 0;
    m_active      = true;
    m_queue.clear();

    // Перечисляем все тайлы для заданной области
    for (int z = minZoom; z <= maxZoom; ++z) {
        int x1 = lonToX(west,  z);
        int x2 = lonToX(east,  z);
        int y1 = latToY(north, z); // north → меньший индекс Y
        int y2 = latToY(south, z); // south → больший индекс Y
        if (x1 > x2) qSwap(x1, x2);
        if (y1 > y2) qSwap(y1, y2);

        for (int x = x1; x <= x2; ++x) {
            for (int y = y1; y <= y2; ++y) {
                QString dir  = QString("%1/%2/%3").arg(outputDir).arg(z).arg(x);
                QString path = QString("%1/%2.png").arg(dir).arg(y);
                if (QFile::exists(path)) continue; // уже скачан — пропускаем
                m_queue.enqueue({z, x, y, path});
                ++m_total;
            }
        }
    }

    if (m_queue.isEmpty()) {
        emit logMessage("Все тайлы уже скачаны.");
        emit finished(0, 0);
        m_active = false;
        return;
    }

    emit logMessage(QString("Запуск скачивания: %1 тайлов (zoom %2–%3)")
                    .arg(m_total).arg(minZoom).arg(maxZoom));
    emit progressChanged(0, m_total);

    m_throttleTimer->start();
    processQueue(); // первый запрос без задержки
}

void MapTileDownloader::cancel()
{
    m_throttleTimer->stop();
    m_queue.clear();
    m_active = false;
    emit logMessage("Скачивание отменено.");
}

void MapTileDownloader::processQueue()
{
    while (m_inFlight < MAX_IN_FLIGHT && !m_queue.isEmpty()) {
        TileTask task = m_queue.dequeue();

        // Создаём директорию
        QDir().mkpath(QFileInfo(task.filePath).absolutePath());

        QString url = QString("https://tile.openstreetmap.org/%1/%2/%3.png")
                          .arg(task.z).arg(task.x).arg(task.y);

        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, "MMK/1.0 (offline map downloader)");
        req.setAttribute(QNetworkRequest::User, task.filePath);
        // Не использовать кэш — хотим всегда свежие данные
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::AlwaysNetwork);

        m_nam->get(req);
        ++m_inFlight;
    }

    if (m_queue.isEmpty() && m_inFlight == 0) {
        m_throttleTimer->stop();
    }
}

void MapTileDownloader::onReplyFinished(QNetworkReply *reply)
{
    --m_inFlight;
    QString filePath = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() == QNetworkReply::NoError) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            ++m_downloaded;
        } else {
            qWarning() << "MapTileDownloader: не удалось записать" << filePath;
            ++m_failed;
        }
    } else {
        qWarning() << "MapTileDownloader: ошибка загрузки" << reply->url().toString()
                   << reply->errorString();
        ++m_failed;
    }

    reply->deleteLater();
    emit progressChanged(m_downloaded + m_failed, m_total);

    if (m_queue.isEmpty() && m_inFlight == 0) {
        m_throttleTimer->stop();
        m_active = false;
        emit logMessage(QString("Скачивание завершено: %1 загружено, %2 ошибок")
                        .arg(m_downloaded).arg(m_failed));
        emit finished(m_downloaded, m_failed);
    }
}
