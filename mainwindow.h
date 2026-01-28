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
#include "zedf9preceiver.h"
#include "amshandler.h"
#include "binshandler.h"

#include "Map/InitialParameters.h"
#include "Map/FormMapView.h"
#include "sensorsettings.h"

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

    ZedF9PReceiver* getGnssReceiver() { return m_gnssReceiver; }
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

    // Настройки датчиков
    void onSensorSettingsClicked();
    void onConnectRequested();
    void onDisconnectRequested();

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

    ZedF9PReceiver *m_gnssReceiver;

    QString m_gnssComPort;
    int m_gnssBaudRate;

    // АМС
    AMSHandler *m_amsHandler;
    QString m_amsComPort;
    int m_amsBaudRate;

    // БИНС
    BINSHandler *m_binsHandler;
    QString m_binsComPort;
    int m_binsBaudRate;

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

    // АМС методы
    void setupAmsHandler();
    void configureAmsDatabase();

    void setupBinsHandler();

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
