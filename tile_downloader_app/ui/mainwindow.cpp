#include "mainwindow.h"
#include "core/tilemath.h"
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QLabel>
#include <QRadioButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QFrame>
#include <QScrollBar>
#include <cmath>

static constexpr qint64 WARN_TILE_THRESHOLD = 500'000;
static constexpr double AVG_TILE_KB         = 25.0;  // honest rough average

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_downloader(new TileDownloader(this))
    , m_previewTimer(new QTimer(this))
{
    setWindowTitle("Tile Downloader — MBTiles Generator");
    setMinimumSize(700, 750);

    buildUi();

    // Debounce preview: 400 ms after last input change
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(400);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::updatePreview);

    // Downloader signals
    connect(m_downloader, &TileDownloader::started,
            this, &MainWindow::onStarted);
    connect(m_downloader, &TileDownloader::progressChanged,
            this, &MainWindow::onProgressChanged);
    connect(m_downloader, &TileDownloader::finished,
            this, &MainWindow::onFinished);
    connect(m_downloader, &TileDownloader::logMessage,
            this, &MainWindow::onLogMessage);

    setControlsEnabled(false);
    updatePreview();
}

// ── UI construction ───────────────────────────────────────────────────────────

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setSpacing(8);
    root->setContentsMargins(10, 10, 10, 10);

    // ── Area selection ────────────────────────────────────────────────────────
    auto *areaBox = new QGroupBox("Geographic Area", central);
    auto *areaV   = new QVBoxLayout(areaBox);

    // Radio buttons
    m_radioBbox   = new QRadioButton("Bounding Box", areaBox);
    m_radioRadius = new QRadioButton("Center + Radius", areaBox);
    m_radioBbox->setChecked(true);
    auto *radioRow = new QHBoxLayout;
    radioRow->addWidget(m_radioBbox);
    radioRow->addWidget(m_radioRadius);
    radioRow->addStretch();
    areaV->addLayout(radioRow);

    // Bbox inputs
    m_bboxGroup = new QGroupBox(areaBox);
    auto *bboxGrid = new QGridLayout(m_bboxGroup);
    m_minLat = new QDoubleSpinBox; m_minLat->setRange(-85.05, 85.05); m_minLat->setDecimals(6); m_minLat->setValue(55.5);
    m_maxLat = new QDoubleSpinBox; m_maxLat->setRange(-85.05, 85.05); m_maxLat->setDecimals(6); m_maxLat->setValue(56.0);
    m_minLon = new QDoubleSpinBox; m_minLon->setRange(-180,   180);   m_minLon->setDecimals(6); m_minLon->setValue(37.0);
    m_maxLon = new QDoubleSpinBox; m_maxLon->setRange(-180,   180);   m_maxLon->setDecimals(6); m_maxLon->setValue(37.8);
    bboxGrid->addWidget(new QLabel("Min Lat:"),  0, 0); bboxGrid->addWidget(m_minLat, 0, 1);
    bboxGrid->addWidget(new QLabel("Max Lat:"),  0, 2); bboxGrid->addWidget(m_maxLat, 0, 3);
    bboxGrid->addWidget(new QLabel("Min Lon:"),  1, 0); bboxGrid->addWidget(m_minLon, 1, 1);
    bboxGrid->addWidget(new QLabel("Max Lon:"),  1, 2); bboxGrid->addWidget(m_maxLon, 1, 3);
    areaV->addWidget(m_bboxGroup);

    // Center+radius inputs
    m_radiusGroup = new QGroupBox(areaBox);
    auto *radGrid = new QGridLayout(m_radiusGroup);
    m_centerLat = new QDoubleSpinBox; m_centerLat->setRange(-85.05, 85.05); m_centerLat->setDecimals(6); m_centerLat->setValue(55.75);
    m_centerLon = new QDoubleSpinBox; m_centerLon->setRange(-180,   180);   m_centerLon->setDecimals(6); m_centerLon->setValue(37.4);
    m_radiusKm  = new QDoubleSpinBox; m_radiusKm->setRange(0.1, 5000);      m_radiusKm->setDecimals(1);  m_radiusKm->setValue(25.0);
    m_btnConvert = new QPushButton("Convert to BBox →");
    radGrid->addWidget(new QLabel("Center Lat:"), 0, 0); radGrid->addWidget(m_centerLat, 0, 1);
    radGrid->addWidget(new QLabel("Center Lon:"), 0, 2); radGrid->addWidget(m_centerLon, 0, 3);
    radGrid->addWidget(new QLabel("Radius (km):"),1, 0); radGrid->addWidget(m_radiusKm,  1, 1);
    radGrid->addWidget(m_btnConvert,              1, 2, 1, 2);
    m_radiusGroup->setVisible(false);
    areaV->addWidget(m_radiusGroup);

    root->addWidget(areaBox);

    // ── Zoom ──────────────────────────────────────────────────────────────────
    auto *zoomBox  = new QGroupBox("Zoom Levels", central);
    auto *zoomForm = new QHBoxLayout(zoomBox);
    m_minZoom = new QSpinBox; m_minZoom->setRange(0, 19); m_minZoom->setValue(5);
    m_maxZoom = new QSpinBox; m_maxZoom->setRange(0, 19); m_maxZoom->setValue(14);
    zoomForm->addWidget(new QLabel("Min zoom:"));
    zoomForm->addWidget(m_minZoom);
    zoomForm->addSpacing(20);
    zoomForm->addWidget(new QLabel("Max zoom:"));
    zoomForm->addWidget(m_maxZoom);
    zoomForm->addStretch();
    root->addWidget(zoomBox);

    // ── Tile source ───────────────────────────────────────────────────────────
    auto *srcBox  = new QGroupBox("Tile Source", central);
    auto *srcForm = new QFormLayout(srcBox);
    m_urlTemplate = new QLineEdit("https://tile.openstreetmap.org/{z}/{x}/{y}.png");
    m_userAgent   = new QLineEdit("TileDownloaderApp/1.0 (contact@example.com)");
    srcForm->addRow("URL Template:", m_urlTemplate);
    srcForm->addRow("User-Agent:",   m_userAgent);
    root->addWidget(srcBox);

    // ── Output ────────────────────────────────────────────────────────────────
    auto *outBox = new QGroupBox("Output File", central);
    auto *outRow = new QHBoxLayout(outBox);
    m_outputPath = new QLineEdit;
    m_outputPath->setPlaceholderText("Select output .mbtiles file...");
    m_btnBrowse  = new QPushButton("Browse...");
    outRow->addWidget(m_outputPath);
    outRow->addWidget(m_btnBrowse);
    root->addWidget(outBox);

    // ── Preview ───────────────────────────────────────────────────────────────
    m_previewLabel = new QLabel("Tiles: —");
    m_previewLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    root->addWidget(m_previewLabel);

    // ── Controls ──────────────────────────────────────────────────────────────
    auto *ctrlRow = new QHBoxLayout;
    m_btnStart  = new QPushButton("▶  Start");
    m_btnPause  = new QPushButton("⏸  Pause");
    m_btnCancel = new QPushButton("✖  Cancel");
    m_btnStart->setFixedHeight(36);
    m_btnPause->setFixedHeight(36);
    m_btnCancel->setFixedHeight(36);
    ctrlRow->addWidget(m_btnStart);
    ctrlRow->addWidget(m_btnPause);
    ctrlRow->addWidget(m_btnCancel);
    root->addLayout(ctrlRow);

    // ── Progress ──────────────────────────────────────────────────────────────
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    root->addWidget(m_progressBar);

    auto *statsRow = new QHBoxLayout;
    m_lblTotal     = new QLabel("Total: 0");
    m_lblDownloaded= new QLabel("Downloaded: 0");
    m_lblFailed    = new QLabel("Failed: 0");
    m_lblSkipped   = new QLabel("Skipped: 0");
    m_lblRemaining = new QLabel("Remaining: 0");
    m_lblFailed->setStyleSheet("color: #c0392b;");
    statsRow->addWidget(m_lblTotal);
    statsRow->addWidget(m_lblDownloaded);
    statsRow->addWidget(m_lblFailed);
    statsRow->addWidget(m_lblSkipped);
    statsRow->addWidget(m_lblRemaining);
    statsRow->addStretch();
    root->addLayout(statsRow);

    // ── Log ───────────────────────────────────────────────────────────────────
    auto *logBox = new QGroupBox("Log", central);
    auto *logV   = new QVBoxLayout(logBox);
    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);
    m_log->setFont(QFont("Monospace", 9));
    logV->addWidget(m_log);
    root->addWidget(logBox, /*stretch=*/1);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_radioBbox,   &QRadioButton::toggled, m_bboxGroup,   &QGroupBox::setVisible);
    connect(m_radioRadius, &QRadioButton::toggled, m_radiusGroup, &QGroupBox::setVisible);

    connect(m_btnStart,   &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_btnPause,   &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(m_btnCancel,  &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    connect(m_btnBrowse,  &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(m_btnConvert, &QPushButton::clicked, this, &MainWindow::onConvertRadiusClicked);

    // Debounced preview on any input change
    auto trigger = [this]() { m_previewTimer->start(); };
    connect(m_minLat,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_maxLat,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_minLon,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_maxLon,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_minZoom, QOverload<int>::of(&QSpinBox::valueChanged),          this, trigger);
    connect(m_maxZoom, QOverload<int>::of(&QSpinBox::valueChanged),          this, trigger);
    connect(m_radioBbox,   &QRadioButton::toggled, this, trigger);
    connect(m_radioRadius, &QRadioButton::toggled, this, trigger);
    connect(m_centerLat, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_centerLon, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
    connect(m_radiusKm,  QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, trigger);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MainWindow::onStartClicked()
{
    double minLat, minLon, maxLat, maxLon;
    int minZ, maxZ;
    if (!collectInputs(minLat, minLon, maxLat, maxLon, minZ, maxZ))
        return;

    QString path = m_outputPath->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Missing output",
                             "Please select an output .mbtiles file.");
        return;
    }

    TileSource src;
    src.urlTemplate = m_urlTemplate->text().trimmed();
    src.userAgent   = m_userAgent->text().trimmed();
    if (!src.isValid()) {
        QMessageBox::warning(this, "Invalid URL",
                             "URL template must contain {z}, {x} and {y}.");
        return;
    }

    qint64 count = TileMath::tileCount(minLat, minLon, maxLat, maxLon, minZ, maxZ);
    if (count > WARN_TILE_THRESHOLD) {
        int r = QMessageBox::warning(this, "Large download",
            QString("This will download approximately %L1 tiles.\n"
                    "This may take a long time and stress the tile server.\n\n"
                    "Continue?").arg(count),
            QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes) return;
    }

    setControlsEnabled(true);
    m_progressBar->setValue(0);
    m_log->clear();

    m_downloader->start(src, path, minLat, minLon, maxLat, maxLon, minZ, maxZ);
}

void MainWindow::onPauseClicked()
{
    if (m_downloader->isPaused()) {
        m_downloader->resume();
        m_btnPause->setText("⏸  Pause");
    } else {
        m_downloader->pause();
        m_btnPause->setText("▶  Resume");
    }
}

void MainWindow::onCancelClicked()
{
    m_downloader->cancel();
}

void MainWindow::onBrowseClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save MBTiles file", QString(),
        "MBTiles (*.mbtiles);;All files (*)");
    if (!path.isEmpty()) {
        if (!path.endsWith(".mbtiles")) path += ".mbtiles";
        m_outputPath->setText(path);
    }
}

void MainWindow::onConvertRadiusClicked()
{
    // Convert center+radius to bbox using flat-earth approximation (good enough for UI)
    double lat = m_centerLat->value();
    double lon = m_centerLon->value();
    double r   = m_radiusKm->value();
    double dLat = r / 111.32;
    double dLon = r / (111.32 * std::cos(lat * M_PI / 180.0));
    m_minLat->setValue(lat - dLat);
    m_maxLat->setValue(lat + dLat);
    m_minLon->setValue(lon - dLon);
    m_maxLon->setValue(lon + dLon);
    m_radioBbox->setChecked(true);
}

void MainWindow::onInputChanged()
{
    m_previewTimer->start();
}

void MainWindow::onStarted(qint64 total, qint64 skipped)
{
    m_totalTiles = total;
    m_progressBar->setValue(0);
    m_lblTotal->setText(QString("Total: %L1").arg(total));
    m_lblSkipped->setText(QString("Skipped: %L1").arg(skipped));
    m_lblDownloaded->setText("Downloaded: 0");
    m_lblFailed->setText("Failed: 0");
    m_lblRemaining->setText(QString("Remaining: %L1").arg(total - skipped));
}

void MainWindow::onProgressChanged(qint64 done, qint64 total, qint64 failed, qint64 skipped)
{
    qint64 processed = done + failed + skipped;
    int pct = (total > 0) ? int(processed * 100 / total) : 0;
    m_progressBar->setValue(pct);
    m_lblDownloaded->setText(QString("Downloaded: %L1").arg(done));
    m_lblFailed->setText(QString("Failed: %L1").arg(failed));
    m_lblSkipped->setText(QString("Skipped: %L1").arg(skipped));
    m_lblTotal->setText(QString("Total: %L1").arg(total));
    qint64 remaining = qMax(0LL, total - processed);
    m_lblRemaining->setText(QString("Remaining: %L1").arg(remaining));
}

void MainWindow::onFinished(qint64 downloaded, qint64 failed, qint64 skipped)
{
    setControlsEnabled(false);
    m_progressBar->setValue(100);
    m_btnPause->setText("⏸  Pause");

    QString msg;
    if (failed == 0) {
        msg = QString("Done! Downloaded %L1 tiles, skipped %L2.").arg(downloaded).arg(skipped);
    } else {
        msg = QString("Done. Downloaded %L1, failed %L2, skipped %L3.")
              .arg(downloaded).arg(failed).arg(skipped);
    }
    statusBar()->showMessage(msg, 10000);
}

void MainWindow::onLogMessage(const QString &msg)
{
    appendLog(msg);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::updatePreview()
{
    double minLat, minLon, maxLat, maxLon;
    int minZ, maxZ;
    if (!collectInputs(minLat, minLon, maxLat, maxLon, minZ, maxZ)) {
        m_previewLabel->setText("Tiles: invalid input");
        return;
    }
    qint64 count = TileMath::tileCount(minLat, minLon, maxLat, maxLon, minZ, maxZ);
    double mb    = count * AVG_TILE_KB / 1024.0;

    QString text;
    if (mb < 1024.0)
        text = QString("~%L1 tiles  (~%2 MB, rough estimate)").arg(count).arg(mb, 0, 'f', 0);
    else
        text = QString("~%L1 tiles  (~%2 GB, rough estimate)").arg(count).arg(mb/1024.0, 0, 'f', 1);

    if (count > WARN_TILE_THRESHOLD)
        text += "  ⚠️ Very large!";

    m_previewLabel->setText(text);
}

void MainWindow::setControlsEnabled(bool downloading)
{
    m_btnStart->setEnabled(!downloading);
    m_btnPause->setEnabled(downloading);
    m_btnCancel->setEnabled(downloading);
    m_bboxGroup->setEnabled(!downloading);
    m_radiusGroup->setEnabled(!downloading);
    m_radioBbox->setEnabled(!downloading);
    m_radioRadius->setEnabled(!downloading);
    m_minZoom->setEnabled(!downloading);
    m_maxZoom->setEnabled(!downloading);
    m_urlTemplate->setEnabled(!downloading);
    m_userAgent->setEnabled(!downloading);
    m_outputPath->setEnabled(!downloading);
    m_btnBrowse->setEnabled(!downloading);
}

void MainWindow::appendLog(const QString &line)
{
    m_log->appendPlainText(line);
    // Auto-scroll to bottom
    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}

bool MainWindow::collectInputs(double &minLat, double &minLon,
                               double &maxLat, double &maxLon,
                               int    &minZ,   int    &maxZ)
{
    if (m_radioBbox->isChecked()) {
        minLat = m_minLat->value();
        maxLat = m_maxLat->value();
        minLon = m_minLon->value();
        maxLon = m_maxLon->value();
    } else {
        // Use last-converted bbox values
        minLat = m_minLat->value();
        maxLat = m_maxLat->value();
        minLon = m_minLon->value();
        maxLon = m_maxLon->value();
    }

    if (minLat >= maxLat || minLon >= maxLon) return false;

    minZ = m_minZoom->value();
    maxZ = m_maxZoom->value();
    if (minZ > maxZ) qSwap(minZ, maxZ);
    return true;
}
