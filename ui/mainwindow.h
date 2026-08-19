#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QQuickWidget>
#include <QComboBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QFrame>
#include <functional>
#include "qmlcoordinateproxy.h"
#include "sensors/gnsshandler.h"
#include "sensors/amshandler.h"
#include "sensors/binshandler.h"
#include "Map/InitialParameters.h"
#include "Map/FormMapView.h"
#include "sensorsettings.h"
#include "surfacemeteosaver.h"
#include "functionalcontroldialog.h"
#include "workregulationdialog.h"
#include "autoconnector.h"
#include "LocalTileServer.h"
#include "calculationAlgorithms/windprofilecalculator.h"
#include "sensors/GroundMeteoParams.h"   // для типа GroundMeteoParams::SurfaceState в слоте


// Forward declaration
class SourceData;
class AlgorithmsCalculation;
class LandingCalculation;

namespace Ui {
class MainWindow;
}

// ─────────────────────────────────────────────────────────────────────────
// ClickableFrame — обычный QFrame с сигналом clicked(). Реализовано через
// прямое переопределение mousePressEvent/mouseReleaseEvent (НЕ через
// installEventFilter — на этой платформе перехват мышиных событий через
// eventFilter уже приводил к зависанию приложения в другом месте интерфейса,
// поэтому для кликабельных элементов используем только штатный, надёжный
// путь — тот же, что использует сам Qt внутри QAbstractButton).
// ─────────────────────────────────────────────────────────────────────────
class ClickableFrame : public QFrame
{
    Q_OBJECT
public:
    explicit ClickableFrame(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_pressed = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isMapCoordinatsEnabled() const { return m_mapCoordinatesEnabled; }
    void updateCoordinatesFromMap(double latitude, double longitude);

    GNSSHandler* getGnssHandler() { return m_gnssHandler; }
    bool isGnssEnabled() const { return m_gnssEnabled; }

    // Геттер для АМС
    AMSHandler* getAmsHandler() { return m_amsHandler; }
    bool isAmsConnected() const { return m_amsHandler && m_amsHandler->isConnected(); }

signals:
    void mapCoordinatesModeChanged(bool enabled);
    void coordinatesUpdatedFromMap(double latitude, double longitude);
    void gnssDataSourceChanged(bool enabled);

private slots:
    // Навигация по страницам главного экрана (лаунчер для планшета)
    void onOpenPositionPage();
    void onOpenMapPage();
    void onOpenMeasurePage();
    void onBackToHome();

    void onFunctionalControlClicked();
    void onWorkRegulationClicked();
    void onManualInputClicked();
    void onInitialDataClicked();
    void onCalculationsClicked();
    void onMeasurementResultsClicked();
    void onStartClicked();
    void onStopClicked();
    // void onWorkModeChanged(bool checked);
    // void onStandbyModeChanged(bool checked);
    void updateDateTime();
    void onSyncTimeClicked();
    void onDateTimeEditingFinished();
    void onDateTimeEditingStarted();

    void onMapCoordinatesToggled();

    void onGnssCheckboxToggled(bool checked);

    // GNSS слоты
    void onGnssDataReceived(const GNSSData &data);
    void onGnssConnected();
    void onGnssDisconnected();
    void onGnssError(const QString &error);
    void onNmeaReceived(const QString &nmea);
    void onGnssConnectFromSettings();
    void onGnssDisconnectFromSettings();

    // Подключение датчиков
    void onConnectSensorsClicked();
    void onConnectRequested();
    void onDisconnectRequested();

    // AutoConnector слоты
    void onAutoConnectorDeviceDetected(AutoConnector::DeviceType type, const QString &port, int baudRate);
    void onAutoConnectorFinished();

    // RS485
    void onSerialDataReceived();
    void onSerialError(QSerialPort::SerialPortError error);
    void pollMeteoStation();

    // АМС слоты
    void onAmsConnectFromSettings();
    void onAmsDisconnectFromSettings();
    void onAmsConnected();
    void onAmsDisconnected();
    void onAmsError(const QString &error);
    void onAmsStatusMessage(const QString &message);
    void onAmsMeasurementProgress(int percent, float angle);
    void onAmsDataWritten(int recordId);
    void onAmsDatabaseError(const QString &error);
    void onAmsMeasurementStageChanged(MeasurementStage stage, const QString &description);
    void onAmsMeasurementCompleted(int recordId);
    void onAmsMeasurementFailed(const QString &reason);
    void onAmsNeedIntermediateData(int progress);
    void onAmsAvgWindReceived(const QVector<WindProfileData> &data);
    void onAmsActualWindReceived(const QVector<WindProfileData> &data);
    void onAmsMeasuredWindReceived(const QVector<MeasuredWindData> &data);

    // ИВС прогрев и проверка подключения
    void onIwsWarmupFinished();
    void onIwsConnectTimeout();

    // Реакция на изменение состояния приземных данных
    // (NoData / Fresh / Stale — приходит от GroundMeteoParams).
    void onSurfaceStateChanged(GroundMeteoParams::SurfaceState newState);

    // БИНС слоты
    void onBinsConnectFromSettings();
    void onBinsDisconnectFromSettings();
    void onBinsConnected();
    void onBinsDisconnected();
    void onBinsError(const QString &error);
    void onBinsStatusMessage(const QString &message);
    void onBinsDataReceived(const BINSData &data);

    // Методы для работы с прогрессом подключения датчиков
    void setupToastUI();
    void showToast();
    void hideToast();
    void repositionToast();

    // Слоты для работы с AutoConnector
    void onAutoConnectorStarted();
    void onAutoConnectorProgress(int current, int total);
    void onAutoConnectorLog(const QString &msg);

    // Остановка поиска датчиков (крестик на toast) и отключение датчика из
    // шторки — оба идут через ОДНО обобщённое окно подтверждения.
    void onToastCloseClicked();
    void onConfirmOverlayAccepted();
    void onConfirmOverlayCancelled();

    // Шторка состояния/управления датчиком (клик по плашке статуса)
    void onSensorPopupActionClicked();

    // Всплывающая карточка при клике на индикатор состояния приземных данных
    void onReadinessIndicatorClicked();

private:
    Ui::MainWindow *ui;
    AutoConnector *m_autoConnector = nullptr;
    QTimer *timer;
    QTimer *pollTimer;
    QSerialPort *serialPort;
    SensorSettings *sensorSettingsDialog;
    SourceData *sourceDataInstance;
    AlgorithmsCalculation *m_algorithmsCalcWidget = nullptr;
    LandingCalculation *m_landingCalcWidget = nullptr;

    QDateTime m_manualDateTime;
    bool m_useManualDateTime;
    bool m_isEditingDateTime;
    bool m_manualDateTimeSet;

    QmlCoordinateProxy qcp;

    FormMapView *fMapView;
    QQuickWidget *m_quickWidget;
    QQuickItem *m_gnssMarker;

    bool m_mapCoordinatesEnabled;
    bool m_gnssEnabled;
    bool m_manualInputEnabled;

    GNSSHandler *m_gnssHandler;

    QString m_gnssComPort;
    int m_gnssBaudRate;

    // АМС
    AMSHandler *m_amsHandler;
    QString m_amsComPort;
    int m_amsBaudRate;

    FunctionalControlDialog *m_functionalControlDialog;

    // БИНС
    BINSHandler *m_binsHandler;
    QString m_binsComPort;
    int m_binsBaudRate;

    // Прогрев ИВС — ожидание 3 минут после подключения
    QTimer *m_iwsWarmupTimer;
    bool    m_iwsWarmupDone;

    // Верификация подключения ИВС — порт открыт ≠ устройство отвечает
    QTimer *m_iwsConnectTimer = nullptr;  // таймаут ожидания первого ответа
    bool    m_iwsDeviceActive = false;    // true только после получения реального ответа

    // Карта: директория кэша тайлов
    QString m_mapCacheDir;

    // Локальный HTTP-сервер тайлов (MBTiles → OSM plugin)
    LocalTileServer *m_tileServer = nullptr;
    QStringList      m_osmMapTypeNames;  // OSM-типы, полученные от плагина
    QString          m_currentMbtilesPath; // пусто = онлайн-режим
    int              m_osmCurrentIndex = 0;

    // Финальный запрос к ИВС по завершении измерения АМС
    int     m_pendingIwsRecordId;   // record_id ожидающий данных ИВС
    QTimer *m_iwsFinalRequestTimer; // таймаут ожидания ответа
    void requestIwsDataForRecord(int recordId);
    void onIwsFinalDataReceived(const QMap<QString, double> &values);

    // Сохранение приземных данных ИВС в БД
    SurfaceMeteoSaver *m_surfaceMeteoSaver;

    // Элементы окна прогресса кноки "Подключить датчики"
    QWidget *m_toastWidget = nullptr;
    QLabel *m_toastTitle = nullptr;
    QLabel *m_toastPercent = nullptr;
    QLabel *m_toastText = nullptr;
    QProgressBar *m_toastProgress = nullptr;
    QPropertyAnimation *m_toastAnimation = nullptr;
    QTimer *m_toastHideTimer = nullptr;
    QPushButton *m_toastCloseBtn = nullptr; // маленькая красная кнопка остановки поиска

    // Окно подтверждения (оверлей поверх всего окна) — ОБЩЕЕ для остановки
    // поиска датчиков и для отключения датчика из шторки. Что именно
    // подтверждается — определяется текстом и колбэком, передаваемыми в
    // showConfirmOverlay(), сама карточка переиспользуется.
    QWidget *m_stopConfirmOverlay = nullptr;
    QWidget *m_stopConfirmCard = nullptr;
    QLabel *m_confirmTitleLabel = nullptr;
    QLabel *m_confirmSubtitleLabel = nullptr;
    QPushButton *m_confirmYesBtn = nullptr;
    QPushButton *m_confirmNoBtn = nullptr;
    QGraphicsOpacityEffect *m_stopConfirmOpacity = nullptr;
    QPropertyAnimation *m_stopConfirmAnimation = nullptr;
    std::function<void()> m_confirmCallback; // выполняется по "Да", затем очищается

    void setupConfirmOverlay();
    void showConfirmOverlay(const QString &title, const QString &subtitle,
                            std::function<void()> onConfirm,
                            const QString &yesLabel = "Да",
                            const QString &noLabel = "Нет");
    void hideConfirmOverlay();

    // ── Шторка состояния/управления датчиком ────────────────────────────────
    // Открывается кликом по одной из 4 плашек статуса (GNSS/АМС/БИНС/ИВС).
    // Показывает состояние, доп. информацию (если доступна по протоколу),
    // причину проблемы при отсутствии связи и кнопку "Подключить" (только
    // когда датчик не подключён — отключать вручную не нужно по задаче:
    // отключение отслеживается автоматически через health-check).
    QWidget *m_sensorPopup = nullptr;
    QLabel *m_sensorPopupTitle = nullptr;
    QLabel *m_sensorPopupStatus = nullptr;
    QLabel *m_sensorPopupInfo = nullptr;
    QLabel *m_sensorPopupReason = nullptr;
    QPushButton *m_sensorPopupActionBtn = nullptr;
    QPropertyAnimation *m_sensorPopupAnimation = nullptr;
    AutoConnector::DeviceType m_currentPopupSensor = AutoConnector::DEVICE_UNKNOWN;

    void setupSensorPopup();
    void showSensorPopup(AutoConnector::DeviceType type);
    void hideSensorPopup();
    void populateSensorPopupContent();
    void startSingleSensorSearch(AutoConnector::DeviceType type);
    // Вызывается ТОЛЬКО изнутри health-check при реальной потере связи
    // (не из UI — кнопки отключения в шторке больше нет).
    void disconnectSensor(AutoConnector::DeviceType type);
    QString sensorDisplayName(AutoConnector::DeviceType type) const;
    QString sensorExtraInfo(AutoConnector::DeviceType type) const;
    // Текст причины проблемы: последняя реальная ошибка от хендлера, если
    // она есть, иначе общий чек-лист "проверьте кабель/порт/скорость".
    QString sensorProblemReason(AutoConnector::DeviceType type) const;
    // "Последние данные: N сек назад" — для GNSS/БИНС/ИВС (вещательные протоколы)
    QString sensorLastSeenText(AutoConnector::DeviceType type) const;
    bool isSensorConnected(AutoConnector::DeviceType type) const;
    QWidget* sensorIndicatorWidget(AutoConnector::DeviceType type) const;

    // ── Health-check: опрос "жив ли датчик" для уже подключённых ───────────
    // АМС — активный (LINE_TEST раз в 5с, только когда не идёт измерение).
    // GNSS/БИНС/ИВС — пассивный "сторож": если дольше kSilenceTimeoutMs не
    // пришло ни байта — считаем отключённым.
    static constexpr int kHealthCheckIntervalMs = 5000;
    static constexpr int kSilenceTimeoutMs = 12000;

    QTimer *m_amsHealthTimer = nullptr;
    QTimer *m_silenceWatchdogTimer = nullptr;

    QDateTime m_gnssLastDataAt;
    QDateTime m_binsLastDataAt;
    QDateTime m_iwsLastDataAt;

    // Последняя реальная ошибка от каждого хендлера — используется как
    // причина в шторке. Очищается при успешном подключении.
    QString m_gnssLastError;
    QString m_amsLastError;
    QString m_binsLastError;
    QString m_iwsLastError;

    void setupHealthChecks();
    void onAmsHealthCheckTimer();
    void onSilenceWatchdogTimer();

    // Toast по итогам поиска — вынесено в отдельный метод, т.к. вызывается
    // и сразу из onAutoConnectorFinished(), и с отсрочкой (см. там же —
    // обходит гонку подтверждения АМС).
    void finalizeAutoConnectorFinished();

    // ── Всплывающая карточка у индикатора состояния приземных данных ───────
    // "Выезжает" из readinessIndicatorFrame (левый верхний угол), по тому же
    // принципу, что и m_toastWidget, но со своей геометрией/анимацией.
    QWidget      *m_readinessPopup = nullptr;
    QLabel       *m_readinessPopupTitle = nullptr;
    QLabel       *m_readinessPopupSubtitle = nullptr;
    QPushButton  *m_readinessPopupYes = nullptr;
    QPushButton  *m_readinessPopupNo = nullptr;
    QPropertyAnimation *m_readinessPopupAnimation = nullptr;
    // Последнее известное состояние приземных данных — хранится отдельно от
    // текста lblStatus, т.к. во время измерения АМС в lblStatus временно
    // показывается "РАБОТА"/"ОШИБКА" поверх этого состояния.
    GroundMeteoParams::SurfaceState m_lastKnownSurfaceState = GroundMeteoParams::NoData;

    void setupReadinessPopup();
    void showReadinessPopup();
    void hideReadinessPopup();
    void populateReadinessPopupContent();

    void connectSensorsFromConfig();
    bool connectIwsPort(const QString &port, int baudRate, QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                        int protocol, quint8 address, int pollInterval);

    void createMapComponent(const QString &pluginName);
    void setupMapItems(QQuickItem *item);

    // MBTiles / tile-server методы
    void writeProvidersJson(const QString &providersDir, const QString &urlTemplate);
    void refreshMapCombo();
    void onMapComboChanged(int index);
    void applyOnlineMapType(int osmIndex);
    void applyMbtilesFile(const QString &mbtilesPath);

    /**
     * Скачать тайлы для заданной области и сохранить в MapOffline-директорию.
     * После завершения карта будет работать без интернета для этого района.
     * @param north/south/west/east  Границы в градусах
     * @param minZoom/maxZoom        Диапазон уровней масштабирования (рекомендуется 5–14)
     */
    void setupMapCoordinatesButton();
    void updateMapCoordinatesButtonStyle();
    void setupGnssCheckbox();
    void setupGnssSettingsButton();
    void updateCoordinateSource(const QString &source);
    void checkAndDisableConflictingSources(const QString &activeSource);
    void updateFieldsEditability();
    void connectToGnss();
    void disconnectFromGnss();
    void updateGnssMarkerOnMap(double latitude, double longitude);
    void setCoordField(QLineEdit *edit, double dec_deg);
    double getCoordField(QLineEdit *edit, bool &ok) const;
    void onCoordTextEdited(QLineEdit *edit);

    // АМС методы
    void setupAmsHandler();
    void configureAmsDatabase();

    // Расчёт действительного и среднего ветра на основе измеренного ветра от
    // АМС и наземных параметров от ИВС. Вызывается из onIwsFinalDataReceived
    // сразу после успешного сохранения surface_meteo, когда все входные данные
    // для расчёта уже находятся в БД и в RAM.
    WindProfileCalculator *m_windProfileCalculator = nullptr;

    // Единая точка завершения измерения: решает, откуда взять приземный
    // ветер (железный IWS либо ручной ввод) и запускает расчёт профилей.
    void tryFinalizeMeasurement(int recordId);

    // Расчёт профилей по уже собранным данным. surfaceWindSpeed/Dir —
    // наземный ветер (из IWS или ручного ввода).
    void runWindProfileCalculation(int recordId,
                                   double surfaceWindSpeed,
                                   double surfaceWindDirection);

    void setupBinsHandler();

    void openMeasurementResults(int recordId = -1); // -1 = просто открыть, >0 = перейти к записи

    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void repositionMapFloatingControls();
    QList<quint16> getRequestParameters();

    // Методы обновления статуса датчиков на панели
    void updateSensorStatusPanel();
    void updateGnssStatusLabel(bool connected);
    void updateAmsStatusLabel(bool connected);
    void updateBinsStatusLabel(bool connected);
    // Плитка "Готов к запуску" на экране "Пуск измерения" (не влияет на
    // btnStart->setEnabled(...) — только поясняющая надпись для оператора).
    void updateMeasureReadinessLabel();
    void updateIwsStatusLabel(bool connected);

    void runPlowSelfTest();
};


#endif // MAINWINDOW_H
