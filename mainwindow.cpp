#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "SourceData.h"
#include "AlgorithmsCalc.h"
#include "MeasurementResults.h"
#include "GroundMeteoParams.h"
#include <QDateTime>
#include <QTimer>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QMessageBox>
#include <QtPositioning/QGeoCoordinate>
#include <QPushButton>
#include <QCheckBox>
#include <QIcon>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , pollTimer(nullptr)
    , serialPort(nullptr)
    , sensorSettingsDialog(nullptr)
    , sourceDataInstance(nullptr)
    , m_mapCoordinatesEnabled(false)
    , m_gnssEnabled(false)
    , m_manualInputEnabled(false)
    , m_gnssReceiver(new ZedF9PReceiver(this))
{
    ui->setupUi(this);

    fMapView = new FormMapView(this);

    setupMapCoordinatesButton();

    setupGnssCheckbox();

    updateMapCoordinatesButtonStyle();

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
    connect(ui->btnSensorSettings, &QPushButton::clicked, this, &MainWindow::onSensorSettingsClicked);

    // GNSS сигналы
    connect(m_gnssReceiver, &ZedF9PReceiver::dataReceived, this, &MainWindow::onGnssDataReceived);
    connect(m_gnssReceiver, &ZedF9PReceiver::connected, this, &MainWindow::onGnssConnected);
    connect(m_gnssReceiver, &ZedF9PReceiver::disconnected, this, &MainWindow::onGnssDisconnected);
    connect(m_gnssReceiver, &ZedF9PReceiver::errorOccurred, this, &MainWindow::onGnssError);

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

    connect(&qcp, &QmlCoordinateProxy::coordinateFromChanged, [=](const QGeoCoordinate &c){
        if (m_mapCoordinatesEnabled){
            updateCoordinatesFromMap(c.latitude(), c.longitude());
        }
    });

    connect(ui->comboBox_mapTypes, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            &qcp, &QmlCoordinateProxy::setCurrentMapType);

    ui->quickWidget->engine()->rootContext()->setContextProperty("coord", &qcp);
    ui->quickWidget->setSource(QUrl("qrc:/qml/Main.qml"));
    createMapComponent("osm");

    // Создаём окно настроек датчиков
    sensorSettingsDialog = new SensorSettings(this);
    connect(sensorSettingsDialog, &SensorSettings::connectRequested, this, &MainWindow::onConnectRequested);
    connect(sensorSettingsDialog, &SensorSettings::disconnectRequested, this, &MainWindow::onDisconnectRequested);

    // Создаём постоянный экземпляр SourceData (внутри создастся GroundMeteoParams)
    // Не показываем его, просто держим в памяти для доступа к данным
    sourceDataInstance = new SourceData(this);
    qDebug() << "SourceData instance created (with GroundMeteoParams inside)";
}

MainWindow::~MainWindow()
{
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    if (pollTimer) {
        pollTimer->stop();
    }
    if (m_gnssReceiver->isConnected()){
        m_gnssReceiver->disconnectFromReceiver();
    }
    delete ui;
}

void MainWindow::setupMapCoordinatesButton()
{
    m_btnMapCoordinates = new QPushButton(this);
    m_btnMapCoordinates->setFixedSize(40, 40);
    m_btnMapCoordinates->setCheckable(true);
    m_btnMapCoordinates->setToolTip("Использовать координаты с карты");

    QIcon markerIcon(":/dat/images/marker.png");
    m_btnMapCoordinates->setIcon(markerIcon);
    m_btnMapCoordinates->setIconSize(QSize(32, 32));

    m_btnMapCoordinates->setStyleSheet(
                "QPushButton {"
                "   background-color: white;"
                "   border: 2px solid gray;"
                "   border-radius: 20px;"
                "}"
                "QPushButton:hover {"
                "   background-color: #f0f0f0;"
                "}"
    );

    m_btnMapCoordinates->move(width() - 60, 20);
    m_btnMapCoordinates->raise();

    connect(m_btnMapCoordinates, &QPushButton::clicked, this, &MainWindow::onMapCoordinatesToggled);
}

void MainWindow::setupGnssCheckbox()
{
    m_checkboxGnss = new QCheckBox("GNSS", this);
    m_checkboxGnss->setStyleSheet(
        "QCheckBox {"
        "   background-color: white;"
        "   padding: 5px;"
        "   border: 2px solid gray;"
        "   border-radius: 5px;"
        "}"
        "   width: 18px;"
        "   height: 18px;"
        "}"
    );

    m_checkboxGnss->move(width() - 160, 20);
    m_checkboxGnss->raise();

    connect(m_checkboxGnss, &QCheckBox::toggled, this, &MainWindow::onGnssCheckboxToggled);
}

void MainWindow::updateMapCoordinatesButtonStyle()
{
    QIcon markerIcon(":/dat/images/marker.png");
    m_btnMapCoordinates->setIcon(markerIcon);
    m_btnMapCoordinates->setIconSize(QSize(32, 32));

    if (m_mapCoordinatesEnabled) {
        m_btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: #4CAF50;"
            "   border: 3px solid #2E7D32;"
            "   border-radius: 20px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #45a049;"
            "   border: 3px solid #1B5E20;"
            "}"
        );
        m_btnMapCoordinates->setToolTip("Режим координат с карты активен (нажмите для отключения)");
    } else {
        m_btnMapCoordinates->setStyleSheet(
            "QPushButton {"
            "   background-color: white;"
            "   border: 2px solid gray;"
            "   border-radius: 20px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #f0f0f0;"
            "}"
        );
        m_btnMapCoordinates->setToolTip("Использовать координаты с карты (нажмите для включения)");
    }
}

void MainWindow::onMapCoordinatesToggled()
{
    m_mapCoordinatesEnabled = !m_mapCoordinatesEnabled;
    updateMapCoordinatesButtonStyle();
    emit mapCoordinatesModeChanged(m_mapCoordinatesEnabled);

    // Выводим сообщение о смене режима
    QString status = m_mapCoordinatesEnabled ?
        "Режим координат с карты ВКЛЮЧЕН" :
        "Режим координат с карты ВЫКЛЮЧЕН";
    statusBar()->showMessage(status, 3000);
}

void MainWindow::onGnssCheckboxToggled(bool checked)
{
    m_gnssEnabled = checked;

    if (m_gnssEnabled){
        checkAndDisableConflictingSources("gnss");

        QStringList ports = ZedF9PReceiver::getAvailablePorts();
        if (!ports.isEmpty()){
               if (m_gnssReceiver->connectToReceiver(ports.first(), 9600)){
                   updateCoordinateSource("GNSS");
                   statusBar()->showMessage("GNSS приемник подключен", 3000);
               } else {
                   m_gnssEnabled = false;
                   m_checkboxGnss->setChecked(false);
                   QMessageBox::warning(this, "Ошибка", "Не удалось подключиться к GNSS приемнику");
               }
        } else {
            m_gnssEnabled = false;
            m_checkboxGnss->setChecked(false);
            QMessageBox::warning(this, "Ошибка", "Не найдены доступные COM-порты для GNSS");
        }
    } else {
        if (m_gnssReceiver->isConnected()){
            m_gnssReceiver->disconnectFromReceiver();
        }
        updateCoordinateSource("Нет");
        statusBar()->showMessage("GNSS приемник отключен", 3000);
    }

    updateFieldsEditability();
    emit gnssDataSourceChanged(m_gnssEnabled);
}

void MainWindow::onGnssDataReceived(const GNSSData &data)
{
    if (!m_gnssEnabled) {
        return; // Игнорируем данные, если GNSS выключен
    }

    // Обновляем поля координат
    ui->editLatitude->setText(QString::number(data.latitude, 'f', 6));
    ui->editLongitude->setText(QString::number(data.longitude, 'f', 6));
    ui->editAltitude->setText(QString::number(data.altitude, 'f', 2));

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(data.latitude, data.longitude);

    qDebug() << "GNSS данные получены:" << data.latitude << data.longitude << data.altitude;
}

void MainWindow::onGnssConnected()
{
    qDebug() << "GNSS приемник подключен";
    m_checkboxGnss->setStyleSheet(
        "QCheckBox {"
        "   background-color: #E8F5E9;"
        "   padding: 5px;"
        "   border: 2px solid #4CAF50;"
        "   border-radius: 5px;"
        "}"
    );
}

void MainWindow::onGnssDisconnected()
{
    qDebug() << "GNSS приемник отключен";
    m_gnssEnabled = false;
    m_checkboxGnss->setChecked(false);
    m_checkboxGnss->setStyleSheet(
        "QCheckBox {"
        "   background-color: white;"
        "   padding: 5px;"
        "   border: 2px solid gray;"
        "   border-radius: 5px;"
        "}"
    );
    updateFieldsEditability();
}

void MainWindow::onGnssError(const QString &error)
{
    qDebug() << "Ошибка GNSS:" << error;
    statusBar()->showMessage("Ошибка GNSS: " + error, 5000);
}

void MainWindow::checkAndDisableConflictingSources(const QString &activeSource)
{
    if (activeSource == "map") {
        // Отключаем GNSS и ручной ввод
        if (m_gnssEnabled) {
            m_checkboxGnss->setChecked(false);
        }
        m_manualInputEnabled = false;
    } else if (activeSource == "gnss") {
        // Отключаем карту и ручной ввод
        if (m_mapCoordinatesEnabled) {
            m_btnMapCoordinates->setChecked(false);
            onMapCoordinatesToggled();
        }
        m_manualInputEnabled = false;
    } else if (activeSource == "manual") {
        // Отключаем карту и GNSS
        if (m_mapCoordinatesEnabled) {
            m_btnMapCoordinates->setChecked(false);
            onMapCoordinatesToggled();
        }
        if (m_gnssEnabled) {
            m_checkboxGnss->setChecked(false);
        }
    }
}

void MainWindow::updateCoordinateSource(const QString &source)
{
    QString message = "Источник координат: " + source;
    statusBar()->showMessage(message, 3000);
    qDebug() << message;
}

void MainWindow::updateFieldsEditability()
{
    // Поля редактируемы только если все источники выключены (ручной ввод)
    bool fieldsEditable = !m_mapCoordinatesEnabled && !m_gnssEnabled;

    ui->editLatitude->setReadOnly(!fieldsEditable);
    ui->editLongitude->setReadOnly(!fieldsEditable);
    ui->editAltitude->setReadOnly(!fieldsEditable);

    // Визуальная индикация
    QString style = fieldsEditable ? "" : "background-color: #F5F5F5;";
    ui->editLatitude->setStyleSheet(style);
    ui->editLongitude->setStyleSheet(style);
    ui->editAltitude->setStyleSheet(style);
}

void MainWindow::updateCoordinatesFromMap(double latitude, double longitude)
{
    ui->editLatitude->setText(QString::number(latitude, 'f', 6));
    ui->editLongitude->setText(QString::number(longitude, 'f', 6));

    // Передаем сигнал другим окнам
    emit coordinatesUpdatedFromMap(latitude, longitude);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (m_btnMapCoordinates){
        m_btnMapCoordinates->move(width() - 60, 20);
    }
    if (m_checkboxGnss){
        m_checkboxGnss->move(width() - 160, 20);
    }
}

void MainWindow::onSensorSettingsClicked()
{
    if (sensorSettingsDialog) {
        sensorSettingsDialog->show();
        sensorSettingsDialog->raise();
        sensorSettingsDialog->activateWindow();
    }
}

void MainWindow::onConnectRequested()
{
    if (sensorSettingsDialog->getComPort().isEmpty() ||
        sensorSettingsDialog->getComPort() == "Нет доступных портов") {
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

    // Настраиваем порт из настроек
    serialPort->setPortName(sensorSettingsDialog->getComPort());
    serialPort->setBaudRate(sensorSettingsDialog->getBaudRate());
    serialPort->setDataBits(sensorSettingsDialog->getDataBits());
    serialPort->setParity(sensorSettingsDialog->getParity());
    serialPort->setStopBits(sensorSettingsDialog->getStopBits());
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Пытаемся открыть порт
    if (serialPort->open(QIODevice::ReadWrite)) {
        sensorSettingsDialog->setConnectionStatus("Подключено", true);
        sensorSettingsDialog->setConnectionEnabled(false);

        // Настраиваем протокол в GroundMeteoParams (если он уже создан)
        GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
        if (meteoParams) {
            GroundMeteoParams::RS485Protocol protocol =
                (sensorSettingsDialog->getProtocolIndex() == 0) ?
                GroundMeteoParams::UMB_PROTOCOL :
                GroundMeteoParams::MODBUS_RTU;
            meteoParams->setProtocol(protocol);
            meteoParams->setDeviceAddress(sensorSettingsDialog->getDeviceAddress());
            qDebug() << "Protocol configured in GroundMeteoParams";
        } else {
            qDebug() << "GroundMeteoParams not created yet. Will be configured when 'Initial Data' is opened.";
        }

        // Запускаем таймер опроса
        if (!pollTimer) {
            pollTimer = new QTimer(this);
            connect(pollTimer, &QTimer::timeout, this, &MainWindow::pollMeteoStation);
        }
        pollTimer->start(sensorSettingsDialog->getPollInterval() * 1000);

        qDebug() << "RS485 connected on" << sensorSettingsDialog->getComPort();
    } else {
        QMessageBox::critical(this, "Ошибка подключения",
                            QString("Не удалось открыть порт: %1").arg(serialPort->errorString()));
        sensorSettingsDialog->setConnectionStatus("Ошибка подключения", false);
    }
}

void MainWindow::onDisconnectRequested()
{
    if (pollTimer) {
        pollTimer->stop();
    }

    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }

    sensorSettingsDialog->setConnectionStatus("Отключено", false);
    sensorSettingsDialog->setConnectionEnabled(true);

    qDebug() << "RS485 disconnected";
}

void MainWindow::onSerialDataReceived()
{
    if (!serialPort) return;

    QByteArray data = serialPort->readAll();
    qDebug() << "Received data:" << data.toHex(' ');

    // Передаём данные в GroundMeteoParams если он открыт
    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (meteoParams) {
        meteoParams->onDataReceived(data);
    }
}

void MainWindow::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
        qDebug() << "Serial port error:" << serialPort->errorString();

        if (sensorSettingsDialog) {
            sensorSettingsDialog->setConnectionStatus(
                QString("Ошибка: %1").arg(serialPort->errorString()), false);
        }

        if (serialPort->isOpen()) {
            onDisconnectRequested();
        }
    }
}

void MainWindow::pollMeteoStation()
{
    if (!serialPort || !serialPort->isOpen()) {
        qDebug() << "Serial port not open, skipping poll";
        return;
    }

    GroundMeteoParams* meteoParams = GroundMeteoParams::instance();
    if (!meteoParams) {
        qDebug() << "GroundMeteoParams instance not found";
        return;
    }

    // Получаем параметры для запроса
    QList<quint16> params = getRequestParameters();

    // Создаём запрос через GroundMeteoParams
    QByteArray request;
    int protocolIndex = sensorSettingsDialog->getProtocolIndex();

    if (protocolIndex == 0) { // UMB
        request = meteoParams->createUmbReadRequest(params);
    } else { // MODBUS
        request = meteoParams->createModbusReadRequest(params);
    }

    if (request.isEmpty()) {
        qDebug() << "Failed to create request";
        return;
    }

    qint64 written = serialPort->write(request);
    if (written != -1) {
        qDebug() << "Request sent (" << written << "bytes):" << request.toHex(' ');
    } else {
        qDebug() << "Failed to write to serial port";
    }
}

QList<quint16> MainWindow::getRequestParameters()
{
    QList<quint16> params;

    if (sensorSettingsDialog->getProtocolIndex() == 0) { // UMB
        params << 0x0064  // Temperature
               << 0x00C8  // Humidity
               << 0x012C  // Pressure
               << 0x0190  // Wind Speed
               << 0x01F4; // Wind Direction
    } else { // MODBUS
        params << 0x0000
               << 0x0001
               << 0x0002
               << 0x0003
               << 0x0004;
    }

    return params;
}

void MainWindow::createMapComponent(const QString &pluginName)
{
    QQuickItem* main = ui->quickWidget->rootObject();
    if (main) {
        qcp.setMapTypes(QStringList());
        QMetaObject::invokeMethod(main, "createMapComponent", Qt::DirectConnection,
                                  Q_ARG(QVariant, pluginName));
        setupMapItems(main);
    }
}

void MainWindow::setupMapItems(QQuickItem *item)
{
    if (item) {
        item->update();
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

    m_manualInputEnabled = !enabled;

    if (m_manualInputEnabled) {
        checkAndDisableConflictingSources("manual");
        updateCoordinateSource("Ручной ввод");
    }
}

void MainWindow::onInitialDataClicked()
{
    // Показываем уже созданный экземпляр SourceData
    if (sourceDataInstance) {
        sourceDataInstance->show();
        sourceDataInstance->raise();
        sourceDataInstance->activateWindow();
    }
}

void MainWindow::onCalculationsClicked()
{
    AlgorithmsCalculation dialog(this);
    dialog.exec();
}

void MainWindow::onMeasurementResultsClicked()
{
    MeasurementResults *dialog = new MeasurementResults(this);

    dialog->exec();
    delete dialog;
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
