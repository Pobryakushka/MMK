#include "tiledownloader.h"
#include <QNetworkRequest>
#include <QDateTime>

TileDownloader::TileDownloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_dispatchTimer(new QTimer(this))
{
    m_dispatchTimer->setInterval(DISPATCH_INTERVAL);
    connect(m_dispatchTimer, &QTimer::timeout, this, &TileDownloader::dispatchNext);
}

TileDownloader::~TileDownloader()
{
    abortAllReplies();
    m_writer.close();
    delete m_iter;
}

// ── Public slots ──────────────────────────────────────────────────────────────

void TileDownloader::start(const TileSource  &source,
                           const QString     &mbtilsPath,
                           double minLat,    double minLon,
                           double maxLat,    double maxLon,
                           int    minZ,      int    maxZ)
{
    if (m_running) return;

    m_source    = source;
    m_running   = true;
    m_paused    = false;
    m_cancelled = false;
    m_done = m_failed = m_skipped = m_batchCount = 0;
    m_active.clear();
    m_retries.clear();

    // Open MBTiles file
    if (!m_writer.open(mbtilsPath)) {
        log("ERROR: Cannot open MBTiles: " + m_writer.lastError());
        m_running = false;
        emit finished(0, 0, 0);
        return;
    }

    // Pre-load existing tiles for O(1) resume checks (no per-tile SQL query)
    m_existing = m_writer.loadExistingTiles();
    m_skipped  = m_existing.size();

    // Write metadata
    m_writer.setMetadata("name",    "OSM Tiles");
    m_writer.setMetadata("type",    "baselayer");
    m_writer.setMetadata("version", "1");
    m_writer.setMetadata("format",  "png");
    m_writer.setMetadata("minzoom", QString::number(minZ));
    m_writer.setMetadata("maxzoom", QString::number(maxZ));
    m_writer.setMetadata("bounds",  QString("%1,%2,%3,%4")
                         .arg(minLon).arg(minLat).arg(maxLon).arg(maxLat));

    // Build lazy iterator
    delete m_iter;
    m_iter  = new TileRangeIterator(minLat, minLon, maxLat, maxLon, minZ, maxZ);
    m_total = m_iter->totalCount();

    m_writer.beginBatch();

    log(QString("Starting: %1 total tiles, %2 already in DB (will skip)")
        .arg(m_total).arg(m_skipped));

    emit started(m_total, m_skipped);
    emitProgress();

    scheduleDispatch();
}

void TileDownloader::pause()
{
    if (!m_running || m_paused) return;
    m_paused = true;
    m_dispatchTimer->stop();
    log("Paused (in-flight requests will complete)");
}

void TileDownloader::resume()
{
    if (!m_running || !m_paused) return;
    m_paused = false;
    log("Resumed");
    scheduleDispatch();
}

void TileDownloader::cancel()
{
    if (!m_running) return;
    m_cancelled = true;
    m_dispatchTimer->stop();
    log("Cancelling...");
    abortAllReplies();
    // abortAllReplies clears m_active; onReplyFinished handles each abort
    // but we call flushAndFinish once all replies are gone
    if (m_active.isEmpty())
        flushAndFinish();
}

// ── Private: dispatch ─────────────────────────────────────────────────────────

void TileDownloader::scheduleDispatch()
{
    if (!m_paused && !m_cancelled)
        m_dispatchTimer->start();
}

void TileDownloader::dispatchNext()
{
    if (m_paused || m_cancelled) {
        m_dispatchTimer->stop();
        return;
    }

    // Drain ready retries first
    QDateTime now = QDateTime::currentDateTimeUtc();
    for (int i = 0; i < m_retries.size() && m_active.size() < MAX_CONCURRENT; ) {
        if (m_retries[i].retryAfter <= now) {
            RetryJob rj = m_retries.takeAt(i);
            fetchTile(rj.id, rj.retries);
        } else {
            ++i;
        }
    }

    // Fill remaining slots from iterator
    while (m_active.size() < MAX_CONCURRENT && m_iter && m_iter->hasNext()) {
        TileId id = m_iter->next();
        if (m_existing.contains(id)) {
            // Already in DB — skip silently (counted in m_skipped at start)
            continue;
        }
        fetchTile(id, 0);
    }

    // Stop timer when nothing left to dispatch and no active jobs
    bool nothingPending = (!m_iter || !m_iter->hasNext()) && m_retries.isEmpty();
    if (nothingPending && m_active.isEmpty()) {
        m_dispatchTimer->stop();
        flushAndFinish();
    } else if (nothingPending) {
        // Still waiting for in-flight replies; keep timer alive to drain retries
    }
}

// ── Private: networking ───────────────────────────────────────────────────────

void TileDownloader::fetchTile(TileId id, int retries)
{
    QNetworkRequest req(m_source.buildUrl(id));
    req.setHeader(QNetworkRequest::UserAgentHeader, m_source.userAgent);
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply *reply = m_nam->get(req);
    m_active.append({id, retries, reply});

    connect(reply, &QNetworkReply::finished,
            this,  &TileDownloader::onReplyFinished);
}

void TileDownloader::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    // Find matching active job
    TileId id{};
    int    retries = 0;
    bool   found   = false;
    for (int i = 0; i < m_active.size(); ++i) {
        if (m_active[i].reply == reply) {
            id      = m_active[i].id;
            retries = m_active[i].retries;
            m_active.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found) return; // aborted and already cleaned up

    if (m_cancelled) {
        if (m_active.isEmpty()) flushAndFinish();
        return;
    }

    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError && httpCode == 200) {
        handleSuccess(id, reply->readAll());
    } else if (httpCode >= 400 && httpCode < 500) {
        // 4xx: don't retry
        handleFailure(id, retries,
                      QString("HTTP %1").arg(httpCode), /*permanent=*/true);
    } else {
        // Network error or 5xx: may retry
        QString reason = reply->errorString();
        if (httpCode > 0) reason = QString("HTTP %1").arg(httpCode);
        handleFailure(id, retries, reason, /*permanent=*/false);
    }

    emitProgress();

    // If we just freed a slot, try to dispatch immediately
    if (!m_paused && !m_cancelled)
        dispatchNext();
}

void TileDownloader::handleSuccess(TileId id, const QByteArray &data)
{
    if (!m_writer.insertTile(id, data)) {
        log("DB write error: " + m_writer.lastError());
        ++m_failed;
        emit tileFailed(id, "DB write error");
        return;
    }
    ++m_done;
    ++m_batchCount;
    emit tileDownloaded(id);

    if (m_batchCount >= BATCH_SIZE) {
        m_writer.commitBatch();
        m_writer.beginBatch();
        m_batchCount = 0;
    }
}

void TileDownloader::handleFailure(TileId id, int retries,
                                   const QString &reason, bool permanent)
{
    if (!permanent && retries < MAX_RETRIES) {
        int delay = backoffMs(retries);
        log(QString("Retry %1/%2 in %3ms — %4 (%5/%6/%7)")
            .arg(retries + 1).arg(MAX_RETRIES).arg(delay)
            .arg(reason)
            .arg(id.z).arg(id.x).arg(id.y));
        m_retries.append({id, retries + 1,
                          QDateTime::currentDateTimeUtc().addMSecs(delay)});
    } else {
        ++m_failed;
        log(QString("FAILED z=%1/x=%2/y=%3: %4")
            .arg(id.z).arg(id.x).arg(id.y).arg(reason));
        emit tileFailed(id, reason);
    }
}

// ── Private: helpers ──────────────────────────────────────────────────────────

void TileDownloader::flushAndFinish()
{
    m_dispatchTimer->stop();
    m_writer.commitBatch();
    m_writer.close();
    delete m_iter;
    m_iter    = nullptr;
    m_running = false;

    QString summary = m_cancelled
        ? QString("Cancelled. Downloaded %1, failed %2, skipped %3.")
          .arg(m_done).arg(m_failed).arg(m_skipped)
        : QString("Finished. Downloaded %1, failed %2, skipped %3.")
          .arg(m_done).arg(m_failed).arg(m_skipped);
    log(summary);
    emit finished(m_done, m_failed, m_skipped);
}

void TileDownloader::abortAllReplies()
{
    for (auto &job : m_active) {
        job.reply->abort();
        job.reply->deleteLater();
    }
    m_active.clear();
    m_retries.clear();
}

void TileDownloader::emitProgress()
{
    emit progressChanged(m_done, m_total, m_failed, m_skipped);
}

void TileDownloader::log(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    emit logMessage(QString("[%1] %2").arg(ts, msg));
}

int TileDownloader::backoffMs(int attempt)
{
    // 0 → 300ms, 1 → 800ms, 2 → 1800ms
    static const int table[] = {300, 800, 1800};
    if (attempt < 0) attempt = 0;
    if (attempt >= int(sizeof(table)/sizeof(table[0])))
        return table[sizeof(table)/sizeof(table[0]) - 1];
    return table[attempt];
}
