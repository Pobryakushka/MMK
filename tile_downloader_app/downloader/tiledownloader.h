#pragma once
#include "tilesource.h"
#include "core/tileid.h"
#include "core/tilerangeiterator.h"
#include "core/mbtiles.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSet>
#include <QDateTime>
#include <QList>

// Manages tile downloading: queue, rate limiting, retry, MBTiles writing.
//
// Threading model: everything runs in the caller's thread.
//   - QNetworkAccessManager and MBTilesWriter share the same thread → no races.
//   - SQLite is opened and written only here, never from another thread.
//
// Retry policy:
//   - HTTP 4xx (incl. 404)  → no retry, tile marked failed immediately
//   - Network/timeout error  → retry up to MAX_RETRIES times with exponential backoff
//   - Backoff: 300 ms → 800 ms → 1800 ms
//
// Pause vs Cancel:
//   - pause()   → stops dispatch timer; in-flight replies finish normally
//   - resume()  → restarts dispatch timer
//   - cancel()  → aborts all in-flight replies, flushes DB, emits finished()
class TileDownloader : public QObject {
    Q_OBJECT

public:
    static constexpr int MAX_RETRIES       = 2;
    static constexpr int MAX_CONCURRENT    = 3;
    static constexpr int DISPATCH_INTERVAL = 200; // ms between dispatch ticks
    static constexpr int BATCH_SIZE        = 50;  // commit every N tiles

    explicit TileDownloader(QObject *parent = nullptr);
    ~TileDownloader();

    bool isRunning() const { return m_running; }
    bool isPaused()  const { return m_paused;  }

public slots:
    // Start a fresh download session.
    // Existing tiles in the MBTiles file are skipped (resume support).
    void start(const TileSource  &source,
               const QString     &mbtilsPath,
               double minLat,    double minLon,
               double maxLat,    double maxLon,
               int    minZ,      int    maxZ);

    void pause();
    void resume();
    void cancel();

signals:
    // Emitted right after open/load so the UI can show total and skipped counts.
    void started(qint64 total, qint64 skipped);

    // Periodic progress update.
    void progressChanged(qint64 done, qint64 total, qint64 failed, qint64 skipped);

    // Per-tile events (useful for verbose log)
    void tileDownloaded(TileId id);
    void tileFailed(TileId id, QString reason);

    // Timestamped log line.
    void logMessage(QString message);

    // Emitted once when all work is done or cancel() was called.
    void finished(qint64 downloaded, qint64 failed, qint64 skipped);

private slots:
    void dispatchNext();
    void onReplyFinished(QNetworkReply *reply);

private:
    struct ActiveJob {
        TileId         id;
        int            retries;
        QNetworkReply *reply;
    };

    struct RetryJob {
        TileId    id;
        int       retries;
        QDateTime retryAfter;
    };

    QNetworkAccessManager  *m_nam;
    TileSource              m_source;
    MBTilesWriter           m_writer;
    TileRangeIterator      *m_iter     = nullptr;
    QSet<TileId>            m_existing; // pre-loaded for O(1) skip check

    QList<ActiveJob>        m_active;
    QList<RetryJob>         m_retries;

    QTimer                 *m_dispatchTimer;

    qint64 m_total    = 0;
    qint64 m_done     = 0;
    qint64 m_failed   = 0;
    qint64 m_skipped  = 0;
    int    m_batchCount = 0; // tiles written since last commit

    bool m_running   = false;
    bool m_paused    = false;
    bool m_cancelled = false;

    void scheduleDispatch();
    void fetchTile(TileId id, int retries);
    void handleSuccess(TileId id, const QByteArray &data);
    void handleFailure(TileId id, int retries, const QString &reason, bool permanent);
    void flushAndFinish();
    void abortAllReplies();
    void emitProgress();
    void log(const QString &msg);
    static int backoffMs(int attempt); // 0→300, 1→800, 2→1800
};
