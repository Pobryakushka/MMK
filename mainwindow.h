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
#include "GroundMeteoParams.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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

    // RS485 слоты
    void onConnectRS485Clicked();
    void onDisconnectRS485Clicked();
    void onShowMeteoParamsClicked();
    void onSerialDataReceived();
    void onSerialError(QSerialPort::SerialPortError error);
    void pollMeteoStation();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QTimer *pollTimer;
    QSerialPort *serialPort;
    GroundMeteoParams *meteoParamsDialog;

    QmlCoordinateProxy qcp;

    FormMapView *fMapView;
    QQuickWidget *m_quickWidget;

    void createMapComponent(const QString &pluginName);
    void setupRS485();
    void updateRS485Status(const QString &status, bool connected);
    void populateComPorts();
    QList<quint16> getRequestParameters();
};


#endif // MAINWINDOW_H
