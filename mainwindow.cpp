#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SourceData.h"
#include "AlgorithmsCalc.h"
#include "MeasurementResults.h"
#include <QDateTime>
#include <QTimer>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    fMapView = new FormMapView(this);

    // Подключение сигналов к слотам
    connect(ui->btnFunctionalControl, &QPushButton::clicked, this, &MainWindow::onFunctionalControlClicked);
    connect(ui->btnWorkRegulation, &QPushButton::clicked, this, &MainWindow::onWorkRegulationClicked);
    connect(ui->btnManualInput, &QPushButton::clicked, this, &MainWindow::onManualInputClicked);
    connect(ui->btnInitialData, &QPushButton::clicked, this, &MainWindow::onInitialDataClicked);
    connect(ui->btnCalculations, &QPushButton::clicked, this, &MainWindow::onCalculationsClicked);
    connect(ui->btnMeasurementResults, &QPushButton::clicked, this, &MainWindow::onMeasurementResultsClicked);
    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(ui->cbWorkMode, &QCheckBox::stateChanged, this, &MainWindow::onWorkModeChanged);
    connect(ui->cbStandbyMode, &QCheckBox::stateChanged, this, &MainWindow::onStandbyModeChanged);
    ui->editAltitude->setEnabled(false);
    ui->editDirectionAngle->setEnabled(false);
    ui->editLatitude->setEnabled(false);
    ui->editLongitude->setEnabled(false);
    ui->editPitchAngle->setEnabled(false);
    ui->editRollAngle->setEnabled(false);

    // Настройка таймера для обновления времени
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    timer->start(1000);

    // Первоначальная установка времени
    updateDateTime();

    connect(&qcp, &QmlCoordinateProxy::mapTypesChanged, [=](const QStringList &list) {
        qDebug() << "mapTypesChanged received with" << list.size() << "items:" << list;
        ui->comboBox_mapTypes->clear();
        ui->comboBox_mapTypes->addItems(list);
    });

    connect(ui->comboBox_mapTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            &qcp, &QmlCoordinateProxy::setCurrentMapType);

    ui->quickWidget->engine()->rootContext()->setContextProperty("coord", &qcp);
    ui->quickWidget->setSource(QUrl("qrc:/qml/Main.qml"));
    createMapComponent("osm");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createMapComponent(const QString &pluginName)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
//        qcp.setMapTypes(QStringList());
        QMetaObject::invokeMethod(main, "createMapComponent", Qt::DirectConnection,
                                  Q_ARG(QVariant, pluginName));
//        setupMapItems(main);
    }
}

void MainWindow::updateDateTime()
{
    ui->lblDateTime->setText(QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss"));
}

void MainWindow::onFunctionalControlClicked()
{
    // Здесь будет логика функционального контроля
}

void MainWindow::onWorkRegulationClicked()
{
    // Здесь будет логика регламента работы
}

void MainWindow::onManualInputClicked()
{
    bool enabled = ui->editAltitude->isEnabled() &&
                   ui->editDirectionAngle->isEnabled() &&
                   ui->editLatitude->isEnabled() &&
                   ui->editLongitude->isEnabled() &&
                   ui->editPitchAngle->isEnabled() &&
                   ui->editRollAngle->isEnabled();

    ui->editAltitude->setEnabled(!enabled);
    ui->editDirectionAngle->setEnabled(!enabled);
    ui->editLatitude->setEnabled(!enabled);
    ui->editLongitude->setEnabled(!enabled);
    ui->editPitchAngle->setEnabled(!enabled);
    ui->editRollAngle->setEnabled(!enabled);
}

void MainWindow::onInitialDataClicked()
{
    SourceData dialog(this);
    dialog.exec();
}

void MainWindow::onCalculationsClicked()
{
    AlgorithmsCalculation dialog(this);
    dialog.exec();
}

void MainWindow::onMeasurementResultsClicked()
{
    MeasurementResults dialog(this);
    dialog.exec();
}

void MainWindow::onStartClicked()
{
    ui->lblStatus->setText("РАБОТА");
    ui->lblStatus->setStyleSheet("color: blue; font-weight: bold; font-size: 14pt; "
                                  "border: 2px solid blue; padding: 5px; border-radius: 5px;");
}

void MainWindow::onStopClicked()
{
    ui->lblStatus->setText("ГОТОВ");
    ui->lblStatus->setStyleSheet("color: green; font-weight: bold; font-size: 14pt; "
                                  "border: 2px solid green; padding: 5px; border-radius: 5px;");
}

void MainWindow::onWorkModeChanged(int state)
{
    if (state == Qt::Checked) {
        ui->cbStandbyMode->setChecked(false);
    }
}

void MainWindow::onStandbyModeChanged(int state)
{
    if (state == Qt::Checked) {
        ui->cbWorkMode->setChecked(false);
    }
}
