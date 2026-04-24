#ifndef MAPTILEDOWNLOADER_H
#define MAPTILEDOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDir>
#include <QQueue>
#include <cmath>

/**
 * @brief Скачивает тайлы OSM для заданной области и сохраняет
 *        в формат офлайн-директории Qt Location: {dir}/{z}/{x}/{y}.png
 *
 * Соблюдает политику OSM: не более 2 запросов/сек с одного IP.
 */
class MapTileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit MapTileDownloader(QObject *parent = nullptr);

    /**
     * Начать скачивание тайлов для заданного прямоугольника.
     * @param outputDir  Путь к офлайн-директории (то же значение, что в osm.mapping.offline.directory)
     * @param north/south/west/east  Границы области в градусах
     * @param minZoom/maxZoom        Диапазон уровней масштабирования
     */
    void download(const QString &outputDir,
                  double north, double south, double west, double east,
                  int minZoom = 5, int maxZoom = 14);

    void cancel();
    bool isDownloading() const { return m_active; }

signals:
    void progressChanged(int downloaded, int total);
    void finished(int downloaded, int failed);
    void logMessage(const QString &msg);

private slots:
    void processQueue();
    void onReplyFinished(QNetworkReply *reply);

private:
    struct TileTask {
        int z, x, y;
        QString filePath;
    };

    static int lonToX(double lon, int z);
    static int latToY(double lat, int z);

    QNetworkAccessManager *m_nam;
    QTimer *m_throttleTimer;   // 500 мс между запросами (~2 req/s)
    QQueue<TileTask> m_queue;
    QString m_outputDir;
    int m_total = 0;
    int m_downloaded = 0;
    int m_failed = 0;
    int m_inFlight = 0;
    bool m_active = false;
    static constexpr int MAX_IN_FLIGHT = 2; // одновременных запросов
};

#endif // MAPTILEDOWNLOADER_H
