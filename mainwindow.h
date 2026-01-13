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

    void onMapCoordinatesToggled();

    void onGnssCheckboxToggled(bool checked);

//    void onGnssSettingsClicked();

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

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QTimer *pollTimer;
    QSerialPort *serialPort;
    SensorSettings *sensorSettingsDialog;
    SourceData *sourceDataInstance;

    QmlCoordinateProxy qcp;

    FormMapView *fMapView;
    QQuickWidget *m_quickWidget;
    QQuickItem *m_gnssMarker;

    bool m_mapCoordinatesEnabled;
    bool m_gnssEnabled;
    bool m_manualInputEnabled;

    QPushButton *m_btnMapCoordinates;
    QCheckBox *m_checkboxGnss;
//    QPushButton *m_btnGnssSettings;

    ZedF9PReceiver *m_gnssReceiver;

    QString m_gnssComPort;
    int m_gnssBaudRate;

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

    void resizeEvent(QResizeEvent *event);
    QList<quint16> getRequestParameters();
};


#endif // MAINWINDOW_H
