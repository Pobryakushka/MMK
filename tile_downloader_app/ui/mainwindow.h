#pragma once
#include "downloader/tiledownloader.h"
#include <QMainWindow>
#include <QTimer>

class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QPlainTextEdit;
class QLabel;
class QRadioButton;
class QGroupBox;
class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onCancelClicked();
    void onBrowseClicked();
    void onConvertRadiusClicked();
    void onInputChanged();          // debounced preview update

    void onStarted(qint64 total, qint64 skipped);
    void onProgressChanged(qint64 done, qint64 total, qint64 failed, qint64 skipped);
    void onFinished(qint64 downloaded, qint64 failed, qint64 skipped);
    void onLogMessage(const QString &msg);

private:
    void buildUi();
    void updatePreview();
    void setControlsEnabled(bool downloading);
    void appendLog(const QString &line);
    bool collectInputs(double &minLat, double &minLon,
                       double &maxLat, double &maxLon,
                       int    &minZ,   int    &maxZ);

    // ── Widgets ──────────────────────────────────────────────────────────────
    // Area — bbox mode
    QRadioButton  *m_radioBbox;
    QGroupBox     *m_bboxGroup;
    QDoubleSpinBox *m_minLat, *m_maxLat, *m_minLon, *m_maxLon;

    // Area — center+radius mode
    QRadioButton  *m_radioRadius;
    QGroupBox     *m_radiusGroup;
    QDoubleSpinBox *m_centerLat, *m_centerLon, *m_radiusKm;
    QPushButton   *m_btnConvert;

    // Zoom
    QSpinBox      *m_minZoom, *m_maxZoom;

    // Source
    QLineEdit     *m_urlTemplate;
    QLineEdit     *m_userAgent;

    // Output
    QLineEdit     *m_outputPath;
    QPushButton   *m_btnBrowse;

    // Preview
    QLabel        *m_previewLabel;

    // Controls
    QPushButton   *m_btnStart;
    QPushButton   *m_btnPause;
    QPushButton   *m_btnCancel;

    // Progress
    QProgressBar  *m_progressBar;
    QLabel        *m_lblDownloaded;
    QLabel        *m_lblFailed;
    QLabel        *m_lblSkipped;
    QLabel        *m_lblTotal;
    QLabel        *m_lblRemaining;

    // Log
    QPlainTextEdit *m_log;

    // ── State ────────────────────────────────────────────────────────────────
    TileDownloader *m_downloader;
    QTimer         *m_previewTimer; // debounce input changes
    qint64          m_totalTiles = 0;
};
