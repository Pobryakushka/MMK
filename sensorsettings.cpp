#include "sensorsettings.h"
#include "ui_sensorsettings.h"
#include <QDebug>

SensorSettings::SensorSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SensorSettings)
{
    ui->setupUi(this);

    populateComPorts();
    populateGnssPorts();
    setupConnections();

    // Устанавливаем значения по умолчанию
    ui->comboBoxBaudRate->setCurrentText("19200");
    ui->comboBoxDataBits->setCurrentText("8");
    ui->comboBoxParity->setCurrentIndex(0); // Нет
    ui->comboBoxStopBits->setCurrentIndex(0); // 1
    ui->comboBoxProtocol->setCurrentIndex(0); // UMB

    ui->comboBoxGnssBaudRate->setCurrentText("19200");
    ui->comboBoxGnssDataBits->setCurrentText("8");
    ui->comboBoxGnssParity->setCurrentIndex(0); // Нет
    ui->comboBoxGnssStopBits->setCurrentIndex(0); // 1
}

SensorSettings::~SensorSettings()
{
    delete ui;
}

void SensorSettings::setupConnections()
{
    // ИВС
    connect(ui->btnRefreshPorts, &QPushButton::clicked, this, &SensorSettings::onRefreshPortsClicked);
    connect(ui->btnConnect, &QPushButton::clicked, this, &SensorSettings::onConnectClicked);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &SensorSettings::onDisconnectClicked);

    // GNSS
    connect(ui->btnRefreshGnssPorts, &QPushButton::clicked, this, &SensorSettings::onRefreshGnssPortsClicked);
    connect(ui->btnConnectGnss, &QPushButton::clicked, this, &SensorSettings::onConnectGnssClicked);
    connect(ui->btnDisconnectGnss, &QPushButton::clicked, this, &SensorSettings::onDisconnectGnssClicked);

    connect(ui->btnClose, &QPushButton::clicked, this, &SensorSettings::onCloseClicked);
}

void SensorSettings::populateComPorts()
{
    ui->comboBoxComPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString portInfo = QString("%1 (%2)").arg(info.portName()).arg(info.description());
        ui->comboBoxComPort->addItem(portInfo, info.systemLocation());
        qDebug() << "SensorSettings: Найден порт ИВС:" << portInfo << "->" << info.systemLocation();
    }

    if (ui->comboBoxComPort->count() == 0) {
        ui->comboBoxComPort->addItem("Нет доступных портов");
        ui->btnConnect->setEnabled(false);
    } else {
        ui->btnConnect->setEnabled(true);
    }
}

void SensorSettings::populateGnssPorts()
{
    ui->comboBoxGnssPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString portInfo = QString("%1 (%2)").arg(info.portName()).arg(info.description());
        ui->comboBoxGnssPort->addItem(portInfo, info.systemLocation());
        qDebug() << "SensorSettings: Найден порт GNSS:" << portInfo << "->" << info.systemLocation();
    }

    if (ui->comboBoxGnssPort->count() == 0) {
        ui->comboBoxGnssPort->addItem("Нет доступных портов");
        ui->btnConnectGnss->setEnabled(false);
        qDebug() << "SensorSettings: Порты GNSS не найдены";
    } else {
        ui->btnConnectGnss->setEnabled(true);
        qDebug() << "SensorSettings: Найдено портов GNSS:" << ui->comboBoxGnssPort->count();
    }
}

void SensorSettings::onRefreshPortsClicked()
{
    populateComPorts();
}

void SensorSettings::onRefreshGnssPortsClicked()
{
    populateGnssPorts();
}

void SensorSettings::onConnectClicked()
{
    emit connectRequested();
}

void SensorSettings::onDisconnectClicked()
{
    emit disconnectRequested();
}

void SensorSettings::onConnectGnssClicked()
{
    emit connectRequested();
}

void SensorSettings::onDisconnectGnssClicked()
{
    emit disconnectRequested();
}

void SensorSettings::onCloseClicked()
{
    close();
}

QString SensorSettings::getComPort() const
{
    if (ui->tabWidget->currentIndex() == 1) {
    QString port = ui->comboBoxGnssPort->currentData().toString();
    return port;
    }

    QString port = ui->comboBoxComPort->currentData().toString();
    return port;
}

int SensorSettings::getBaudRate() const
{
    if (ui->tabWidget->currentIndex() == 1) {
    int baudRate = ui->comboBoxGnssBaudRate->currentText().toInt();
    return baudRate;
    }
    int baudRate = ui->comboBoxBaudRate->currentText().toInt();
    return baudRate;
}

QSerialPort::DataBits SensorSettings::getDataBits() const
{
    int bits;
    if (ui->tabWidget->currentIndex() == 1) {
        bits = ui->comboBoxGnssDataBits->currentText().toInt();
    } else {
        bits = ui->comboBoxDataBits->currentText().toInt();
    }
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getParity() const
{
    int index;
    if (ui->tabWidget->currentIndex() == 1) {
        index = ui->comboBoxGnssParity->currentIndex();
    } else {
        index = ui->comboBoxParity->currentIndex();
    }

    switch (index) {
        case 1: return QSerialPort::EvenParity;
        case 2: return QSerialPort::OddParity;
        default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getStopBits() const
{
    int index;
    if (ui->tabWidget->currentIndex() == 1) {
        index = ui->comboBoxGnssStopBits->currentIndex();
    } else {
        index = ui->comboBoxStopBits->currentIndex();
    }
    return (index == 0) ? QSerialPort::OneStop : QSerialPort::TwoStop;
}

int SensorSettings::getProtocolIndex() const
{
    return ui->comboBoxProtocol->currentIndex();
}

quint8 SensorSettings::getDeviceAddress() const
{
    return static_cast<quint8>(ui->spinBoxDeviceAddress->value());
}

int SensorSettings::getPollInterval() const
{
    return ui->spinBoxPollInterval->value();
}

void SensorSettings::setConnectionStatus(const QString& status, bool connected)
{
    // Определяем активную вкладку
    if (ui->tabWidget->currentIndex() == 1) {
        // GNSS вкладка
        ui->lblGnssStatus->setText(QString("Статус: %1").arg(status));

        if (connected) {
            ui->lblGnssStatus->setStyleSheet("color: green; font-size: 10pt; padding: 5px; font-weight: bold;");
            ui->btnConnectGnss->setEnabled(false);
            ui->btnDisconnectGnss->setEnabled(true);
            ui->btnDisconnectGnss->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
        } else {
            ui->lblGnssStatus->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
            ui->btnConnectGnss->setEnabled(true);
            ui->btnDisconnectGnss->setEnabled(false);
            ui->btnDisconnectGnss->setStyleSheet("background-color: #757575; color: white; font-weight: bold;");
        }
    } else {
        // ИВС вкладка
        ui->lblConnectionStatus->setText(QString("Статус: %1").arg(status));

        if (connected) {
            ui->lblConnectionStatus->setStyleSheet("color: green; font-size: 10pt; padding: 5px; font-weight: bold;");
            ui->btnConnect->setEnabled(false);
            ui->btnDisconnect->setEnabled(true);
            ui->btnDisconnect->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
        } else {
            ui->lblConnectionStatus->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
            ui->btnConnect->setEnabled(true);
            ui->btnDisconnect->setEnabled(false);
            ui->btnDisconnect->setStyleSheet("background-color: #757575; color: white; font-weight: bold;");
        }
    }

    qDebug() << "SensorSettings: Статус подключения установлен:" << status << "Connected:" << connected;

}

void SensorSettings::setConnectionEnabled(bool enabled)
{
    // Определяем активную вкладку
    if (ui->tabWidget->currentIndex() == 1) {
        // GNSS вкладка
        ui->comboBoxGnssPort->setEnabled(enabled);
        ui->comboBoxGnssBaudRate->setEnabled(enabled);
        ui->comboBoxGnssDataBits->setEnabled(enabled);
        ui->comboBoxGnssParity->setEnabled(enabled);
        ui->comboBoxGnssStopBits->setEnabled(enabled);
        ui->btnRefreshGnssPorts->setEnabled(enabled);
    } else {
        // ИВС вкладка
        ui->comboBoxComPort->setEnabled(enabled);
        ui->comboBoxBaudRate->setEnabled(enabled);
        ui->comboBoxDataBits->setEnabled(enabled);
        ui->comboBoxParity->setEnabled(enabled);
        ui->comboBoxStopBits->setEnabled(enabled);
        ui->comboBoxProtocol->setEnabled(enabled);
        ui->spinBoxDeviceAddress->setEnabled(enabled);
        ui->spinBoxPollInterval->setEnabled(enabled);
        ui->btnRefreshPorts->setEnabled(enabled);
    }
}
