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
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialPort(nullptr)
    , pollTimer(nullptr)
    , meteoParamsDialog(nullptr)
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

    // Настройка RS485
    setupRS485();
}

MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    if (pollTimer) {
        pollTimer->stop();
    }
    delete ui;
}

void MainWindow::setupRS485()
{
    // Заполняем список COM-портов
    populateComPorts();

    // Устанавливаем значения по умолчанию
    ui->comboBoxBaudRate->setCurrentText("19200");
    ui->comboBoxProtocol->setCurrentIndex(0); // UMB по умолчанию

    // Подключаем кнопки RS485
    connect(ui->btnConnectRS485, &QPushButton::clicked, this, &MainWindow::onConnectRS485Clicked);
    connect(ui->btnDisconnectRS485, &QPushButton::clicked, this, &MainWindow::onDisconnectRS485Clicked);
    connect(ui->btnShowMeteoParams, &QPushButton::clicked, this, &MainWindow::onShowMeteoParamsClicked);

    // Создаём диалог метеопараметров
    meteoParamsDialog = new GroundMeteoParams(this);
}

void MainWindow::populateComPorts()
{
    ui->comboBoxComPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        ui->comboBoxComPort->addItem(info.portName(), info.systemLocation());
    }

    if (ui->comboBoxComPort->count() == 0) {
        ui->comboBoxComPort->addItem("Нет доступных портов");
        ui->btnConnectRS485->setEnabled(false);
    }
}

void MainWindow::onConnectRS485Clicked()
{
    if (ui->comboBoxComPort->count() == 0 ||
        ui->comboBoxComPort->currentText() == "Нет доступных портов") {
        QMessageBox::warning(this, "Ошибка", "Нет доступных COM-портов");
        return;
    }

    // Создаём serial port если ещё не создан
    if (!serialPort) {
        serialPort = new QSerialPort(this);
        connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::onSerialDataReceived);
        connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::onSerialError);
    }

    // Закрываем если уже открыт
    if (serialPort->isOpen()) {
        serialPort->close();
    }

    // Настраиваем порт
    QString portName = ui->comboBoxComPort->currentData().toString();
    serialPort->setPortName(portName);
    serialPort->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Пытаемся открыть порт
    if (serialPort->open(QIODevice::ReadWrite)) {
        // Настраиваем протокол в диалоге
        meteoParamsDialog->setSerialPort(serialPort);

        GroundMeteoParams::RS485Protocol protocol =
            (ui->comboBoxProtocol->currentIndex() == 0) ?
            GroundMeteoParams::UMB_PROTOCOL :
            GroundMeteoParams::MODBUS_RTU;
        meteoParamsDialog->setProtocol(protocol);
        meteoParamsDialog->setDeviceAddress(ui->spinBoxDeviceAddress->value());

        // Обновляем интерфейс
        ui->btnConnectRS485->setEnabled(false);
        ui->btnDisconnectRS485->setEnabled(true);
        ui->comboBoxComPort->setEnabled(false);
        ui->comboBoxBaudRate->setEnabled(false);
        ui->comboBoxProtocol->setEnabled(false);
        ui->spinBoxDeviceAddress->setEnabled(false);

        updateRS485Status("Подключено", true);

        // Запускаем таймер опроса
        if (!pollTimer) {
            pollTimer = new QTimer(this);
            connect(pollTimer, &QTimer::timeout, this, &MainWindow::pollMeteoStation);
        }
        pollTimer->start(ui->spinBoxPollInterval->value() * 1000);

        qDebug() << "RS485 connected on" << portName;
    } else {
        QMessageBox::critical(this, "Ошибка подключения",
                            QString("Не удалось открыть порт: %1").arg(serialPort->errorString()));
        updateRS485Status("Ошибка подключения", false);
    }
}

void MainWindow::onDisconnectRS485Clicked()
{
    if (pollTimer) {
        pollTimer->stop();
    }

    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }

    // Обновляем интерфейс
    ui->btnConnectRS485->setEnabled(true);
    ui->btnDisconnectRS485->setEnabled(false);
    ui->comboBoxComPort->setEnabled(true);
    ui->comboBoxBaudRate->setEnabled(true);
    ui->comboBoxProtocol->setEnabled(true);
    ui->spinBoxDeviceAddress->setEnabled(true);

    updateRS485Status("Отключено", false);

    qDebug() << "RS485 disconnected";
}

void MainWindow::onShowMeteoParamsClicked()
{
    if (meteoParamsDialog) {
        meteoParamsDialog->show();
        meteoParamsDialog->raise();
        meteoParamsDialog->activateWindow();
    }
}

void MainWindow::onSerialDataReceived()
{
    if (serialPort && meteoParamsDialog) {
        QByteArray data = serialPort->readAll();
        meteoParamsDialog->onDataReceived(data);
    }
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        qDebug() << "Serial port error:" << serialPort->errorString();
        updateRS485Status(QString("Ошибка: %1").arg(serialPort->errorString()), false);

        if (serialPort->isOpen()) {
            onDisconnectRS485Clicked();
        }
    }
}

void MainWindow::pollMeteoStation()
{
    if (serialPort && serialPort->isOpen() && meteoParamsDialog) {
        QList<quint16> params = getRequestParameters();
        meteoParamsDialog->requestData(params);
        qDebug() << "Polling meteo station...";
    }
}

QList<quint16> MainWindow::getRequestParameters()
{
    // Параметры для запроса (коды UMB/MODBUS)
    QList<quint16> params;

    if (ui->comboBoxProtocol->currentIndex() == 0) { // UMB
        params << 0x0064  // Temperature (100)
               << 0x00C8  // Humidity (200)
               << 0x012C  // Pressure (300)
               << 0x0190  // Wind Speed (400)
               << 0x01F4; // Wind Direction (500)
    } else { // MODBUS
        // Для MODBUS обычно используются последовательные адреса регистров
        params << 0x0000  // Начальный адрес
               << 0x0001
               << 0x0002
               << 0x0003
               << 0x0004;
    }

    return params;
}

void MainWindow::updateRS485Status(const QString &status, bool connected)
{
    ui->lblRS485Status->setText(QString("Статус: %1").arg(status));

    if (connected) {
        ui->lblRS485Status->setStyleSheet("color: green; font-size: 9pt; padding: 3px; font-weight: bold;");
    } else {
        ui->lblRS485Status->setStyleSheet("color: #666; font-size: 9pt; padding: 3px;");
    }
}

void MainWindow::createMapComponent(const QString &pluginName)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        QMetaObject::invokeMethod(main, "createMapComponent", Qt::DirectConnection,
                                  Q_ARG(QVariant, pluginName));
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
