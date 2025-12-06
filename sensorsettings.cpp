#include "sensorsettings.h"
#include "ui_SensorSettings.h"

SensorSettings::SensorSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SensorSettings)
{
    ui->setupUi(this);

    populateComPorts();
    setupConnections();

    // Устанавливаем значения по умолчанию
    ui->comboBoxBaudRate->setCurrentText("19200");
    ui->comboBoxDataBits->setCurrentText("8");
    ui->comboBoxParity->setCurrentIndex(0); // Нет
    ui->comboBoxStopBits->setCurrentIndex(0); // 1
    ui->comboBoxProtocol->setCurrentIndex(0); // UMB
}

SensorSettings::~SensorSettings()
{
    delete ui;
}

void SensorSettings::setupConnections()
{
    connect(ui->btnRefreshPorts, &QPushButton::clicked, this, &SensorSettings::onRefreshPortsClicked);
    connect(ui->btnConnect, &QPushButton::clicked, this, &SensorSettings::onConnectClicked);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &SensorSettings::onDisconnectClicked);
    connect(ui->btnClose, &QPushButton::clicked, this, &SensorSettings::onCloseClicked);
}

void SensorSettings::populateComPorts()
{
    ui->comboBoxComPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        ui->comboBoxComPort->addItem(info.portName(), info.systemLocation());
    }

    if (ui->comboBoxComPort->count() == 0) {
        ui->comboBoxComPort->addItem("Нет доступных портов");
        ui->btnConnect->setEnabled(false);
    } else {
        ui->btnConnect->setEnabled(true);
    }
}

void SensorSettings::onRefreshPortsClicked()
{
    populateComPorts();
}

void SensorSettings::onConnectClicked()
{
    emit connectRequested();
}

void SensorSettings::onDisconnectClicked()
{
    emit disconnectRequested();
}

void SensorSettings::onCloseClicked()
{
    close();
}

QString SensorSettings::getComPort() const
{
    return ui->comboBoxComPort->currentData().toString();
}

int SensorSettings::getBaudRate() const
{
    return ui->comboBoxBaudRate->currentText().toInt();
}

QSerialPort::DataBits SensorSettings::getDataBits() const
{
    int bits = ui->comboBoxDataBits->currentText().toInt();
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getParity() const
{
    switch (ui->comboBoxParity->currentIndex()) {
        case 1: return QSerialPort::EvenParity;
        case 2: return QSerialPort::OddParity;
        default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getStopBits() const
{
    return (ui->comboBoxStopBits->currentIndex() == 0) ?
           QSerialPort::OneStop : QSerialPort::TwoStop;
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

void SensorSettings::setConnectionEnabled(bool enabled)
{
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
