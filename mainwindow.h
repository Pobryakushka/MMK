#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QQuickWidget>
#include <QComboBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "qmlcoordinateproxy.h"
#include "gnsshandler.h"
#include "amshandler.h"
#include "binshandler.h"
#include "Map/InitialParameters.h"
#include "Map/FormMapView.h"
#include "sensorsettings.h"
#include "surfacemeteosaver.h"
#include "functionalcontroldialog.h"
#include "workregulationdialog.h"
#include "autoconnector.h"

// Forward declaration
class SourceData;

namespace Ui {
class MainWindow;
}

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
    void onFunctionalControlClicked();
    void onWorkRegulationClicked();
    void onManualInputClicked();
    void onInitialDataClicked();
    void onCalculationsClicked();
    void onMeasurementResultsClicked();
    void onStartClicked();
    void onStopClicked();
    void onWorkModeChanged(int state);
    void onStandbyModeChanged(int state);
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

    // БИНС слоты
    void onBinsConnectFromSettings();
    void onBinsDisconnectFromSettings();
    void onBinsConnected();
    void onBinsDisconnected();
    void onBinsError(const QString &error);
    void onBinsStatusMessage(const QString &message);
    void onBinsDataReceived(const BINSData &data);

private:
    Ui::MainWindow *ui;
    AutoConnector *m_autoConnector = nullptr;
    QTimer *timer;
    QTimer *pollTimer;
    QSerialPort *serialPort;
    SensorSettings *sensorSettingsDialog;
    SourceData *sourceDataInstance;

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

    // Финальный запрос к ИВС по завершении измерения АМС
    int     m_pendingIwsRecordId;   // record_id ожидающий данных ИВС
    QTimer *m_iwsFinalRequestTimer; // таймаут ожидания ответа
    void requestIwsDataForRecord(int recordId);
    void onIwsFinalDataReceived(const QMap<QString, double> &values);

    // Сохранение приземных данных ИВС в БД
    SurfaceMeteoSaver *m_surfaceMeteoSaver;

    void connectSensorsFromConfig();
    bool connectIwsPort(const QString &port, int baudRate, QSerialPort::DataBits dataBits,
                        QSerialPort::Parity parity, QSerialPort::StopBits stopBits,
                        int protocol, quint8 address, int pollInterval);

    void createMapComponent(const QString &pluginName);
    void setupMapItems(QQuickItem *item);
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

    void setupBinsHandler();

    void openMeasurementResults(int recordId = -1); // -1 = просто открыть, >0 = перейти к записи

    void resizeEvent(QResizeEvent *event);
    QList<quint16> getRequestParameters();

    // Методы обновления статуса датчиков на панели
    void updateSensorStatusPanel();
    void updateGnssStatusLabel(bool connected);
    void updateAmsStatusLabel(bool connected);
    void updateBinsStatusLabel(bool connected);
    void updateIwsStatusLabel(bool connected);
};


#endif // MAINWINDOW_H
