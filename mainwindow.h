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

signals:
    void mapCoordinatesModeChanged(bool enabled);
    void coordinatesUpdatedFromMap(double latitude, double longitude);

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

    bool m_mapCoordinatesEnabled;
    QPushButton *m_btnMapCoordinates;

    void createMapComponent(const QString &pluginName);
    void setupMapItems(QQuickItem *item);
    void setupMapCoordinatesButton();
    void updateMapCoordinatesButtonStyle();
    void resizeEvent(QResizeEvent *event);
    QList<quint16> getRequestParameters();
};


#endif // MAINWINDOW_H
