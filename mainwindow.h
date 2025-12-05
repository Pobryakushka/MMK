#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QButtonGroup>
#include <QTimer>
#include <QQuickWidget>
#include <QComboBox>
#include "qmlcoordinateproxy.h"

#include "Map/InitialParameters.h"
#include "Map/FormMapView.h"

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

private:
    Ui::MainWindow *ui;
    QTimer *timer;

    QmlCoordinateProxy qcp;

    FormMapView *fMapView;
    QQuickWidget *m_quickWidget;
    void createMapComponent(const QString &pluginName);
};


#endif // MAINWINDOW_H
