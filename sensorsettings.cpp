#include "sensorsettings.h"
#include "ui_sensorsettings.h"
#include <QDebug>
#include <QFile>
#include <QDir>

SensorSettings::SensorSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SensorSettings)
{
    ui->setupUi(this);

    populateComPorts();
    populateGnssPorts();
    populateAmsPorts();
    setupConnections();

    // Устанавливаем значения по умолчанию для ИВС
    ui->comboBoxBaudRate->setCurrentText("19200");
    ui->comboBoxDataBits->setCurrentText("8");
    ui->comboBoxParity->setCurrentIndex(0); // Нет
    ui->comboBoxStopBits->setCurrentIndex(0); // 1
    ui->comboBoxProtocol->setCurrentIndex(0); // UMB

    // Устанавливаем значения по умолчанию для GNSS
    ui->comboBoxGnssBaudRate->setCurrentText("19200");
    ui->comboBoxGnssDataBits->setCurrentText("8");
    ui->comboBoxGnssParity->setCurrentIndex(0); // Нет
    ui->comboBoxGnssStopBits->setCurrentIndex(0); // 1

    // Устанавливаем значения по умолчанию для АМС (RS-422C)
    ui->comboBoxAmsBaudRate->setCurrentText("9600");
    ui->comboBoxAmsDataBits->setCurrentText("8");
    ui->comboBoxAmsParity->setCurrentIndex(0); // Нет
    ui->comboBoxAmsStopBits->setCurrentIndex(0); // 1
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

    // АМС
    connect(ui->btnRefreshAmsPorts, &QPushButton::clicked, this, &SensorSettings::onRefreshAmsPortsClicked);
    connect(ui->btnConnectAms, &QPushButton::clicked, this, &SensorSettings::onConnectAmsClicked);
    connect(ui->btnDisconnectAms, &QPushButton::clicked, this, &SensorSettings::onDisconnectAmsClicked);

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

    // Добавляем виртуальные pts порты (для эмуляторов)
    QDir ptsDir("/dev/pts");
    if (ptsDir.exists()) {
        QStringList ptsFiles = ptsDir.entryList(QDir::System | QDir::Readable | QDir::NoDotAndDotDot);
        for (const QString &ptsFile : ptsFiles) {
            bool isNumber;
            ptsFile.toInt(&isNumber);
            if (isNumber) {  // Только числовые pts (пропускаем ptmx)
                QString ptsPath = "/dev/pts/" + ptsFile;
                ui->comboBoxComPort->addItem(QString("Virtual pts/%1").arg(ptsFile), ptsPath);
                qDebug() << "SensorSettings: Найден виртуальный порт ИВС:" << ptsPath;
            }
        }
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

    // Добавляем виртуальные pts порты (для эмуляторов)
    QDir ptsDir("/dev/pts");
    if (ptsDir.exists()) {
        QStringList ptsFiles = ptsDir.entryList(QDir::System | QDir::Readable | QDir::NoDotAndDotDot);
        for (const QString &ptsFile : ptsFiles) {
            bool isNumber;
            ptsFile.toInt(&isNumber);
            if (isNumber) {  // Только числовые pts (пропускаем ptmx)
                QString ptsPath = "/dev/pts/" + ptsFile;
                ui->comboBoxGnssPort->addItem(QString("Virtual pts/%1").arg(ptsFile), ptsPath);
                qDebug() << "SensorSettings: Найден виртуальный порт GNSS:" << ptsPath;
            }
        }
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

void SensorSettings::populateAmsPorts()
{
    ui->comboBoxAmsPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString portInfo = QString("%1 (%2)").arg(info.portName()).arg(info.description());
        ui->comboBoxAmsPort->addItem(portInfo, info.systemLocation());
        qDebug() << "SensorSettings: Найден порт АМС:" << portInfo << "->" << info.systemLocation();
    }

    // Добавляем виртуальные pts порты (для эмуляторов)
    QDir ptsDir("/dev/pts");
    if (ptsDir.exists()) {
        QStringList ptsFiles = ptsDir.entryList(QDir::System | QDir::Readable | QDir::NoDotAndDotDot);
        for (const QString &ptsFile : ptsFiles) {
            bool isNumber;
            ptsFile.toInt(&isNumber);
            if (isNumber) {  // Только числовые pts (пропускаем ptmx)
                QString ptsPath = "/dev/pts/" + ptsFile;
                ui->comboBoxAmsPort->addItem(QString("Virtual pts/%1").arg(ptsFile), ptsPath);
                qDebug() << "SensorSettings: Найден виртуальный порт АМС:" << ptsPath;
            }
        }
    }

    if (ui->comboBoxAmsPort->count() == 0) {
        ui->comboBoxAmsPort->addItem("Нет доступных портов");
        ui->btnConnectAms->setEnabled(false);
        qDebug() << "SensorSettings: Порты АМС не найдены";
    } else {
        ui->btnConnectAms->setEnabled(true);
        qDebug() << "SensorSettings: Найдено портов АМС:" << ui->comboBoxAmsPort->count();
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

void SensorSettings::onRefreshAmsPortsClicked()
{
    populateAmsPorts();
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
    emit gnssConnectRequested();
}

void SensorSettings::onDisconnectGnssClicked()
{
    emit gnssDisconnectRequested();
}

void SensorSettings::onConnectAmsClicked()
{
    emit amsConnectRequested();
}

void SensorSettings::onDisconnectAmsClicked()
{
    emit amsDisconnectRequested();
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

// ===== МЕТОДЫ ДЛЯ АМС =====

QString SensorSettings::getAmsComPort() const
{
    QString port = ui->comboBoxAmsPort->currentData().toString();
    return port;
}

int SensorSettings::getAmsBaudRate() const
{
    int baudRate = ui->comboBoxAmsBaudRate->currentText().toInt();
    return baudRate;
}

QSerialPort::DataBits SensorSettings::getAmsDataBits() const
{
    int bits = ui->comboBoxAmsDataBits->currentText().toInt();
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getAmsParity() const
{
    int index = ui->comboBoxAmsParity->currentIndex();
    switch (index) {
        case 1: return QSerialPort::EvenParity;
        case 2: return QSerialPort::OddParity;
        default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getAmsStopBits() const
{
    int index = ui->comboBoxAmsStopBits->currentIndex();
    return (index == 0) ? QSerialPort::OneStop : QSerialPort::TwoStop;
}

void SensorSettings::setAmsConnectionStatus(const QString& status, bool connected)
{
    ui->lblAmsStatus->setText(QString("Статус: %1").arg(status));

    if (connected) {
        ui->lblAmsStatus->setStyleSheet("color: green; font-size: 10pt; padding: 5px; font-weight: bold;");
        ui->btnConnectAms->setEnabled(false);
        ui->btnDisconnectAms->setEnabled(true);
        ui->btnDisconnectAms->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
    } else {
        ui->lblAmsStatus->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
        ui->btnConnectAms->setEnabled(true);
        ui->btnDisconnectAms->setEnabled(false);
        ui->btnDisconnectAms->setStyleSheet("background-color: #757575; color: white; font-weight: bold;");
    }

    qDebug() << "SensorSettings: Статус АМС установлен:" << status << "Connected:" << connected;
}

void SensorSettings::setAmsConnectionEnabled(bool enabled)
{
    ui->comboBoxAmsPort->setEnabled(enabled);
    ui->comboBoxAmsBaudRate->setEnabled(enabled);
    ui->comboBoxAmsDataBits->setEnabled(enabled);
    ui->comboBoxAmsParity->setEnabled(enabled);
    ui->comboBoxAmsStopBits->setEnabled(enabled);
    ui->btnRefreshAmsPorts->setEnabled(enabled);
}

// ===== СУЩЕСТВУЮЩИЕ МЕТОДЫ =====

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
