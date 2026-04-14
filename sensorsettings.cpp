#include "sensorsettings.h"
#include "ui_sensorsettings.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>

SensorSettings::SensorSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SensorSettings)
    , m_autoConnector(new AutoConnector(this))
{
    ui->setupUi(this);

    populateComPorts();
    populateGnssPorts();
    populateAmsPorts();
    populateBinsPorts();
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

    // Устанавливаем значения по умолчанию для БИНС (RS-232)
    ui->comboBoxBinsBaudRate->setCurrentText("115200");
    ui->comboBoxBinsDataBits->setCurrentText("8");
    ui->comboBoxBinsParity->setCurrentIndex(0); // Нет
    ui->comboBoxBinsStopBits->setCurrentIndex(0); // 1

    // Подключаем сигналы AutoConnector
    connect(m_autoConnector, &AutoConnector::detectionStarted,
            this, &SensorSettings::onAutoDetectionStarted);
    connect(m_autoConnector, &AutoConnector::detectionFinished,
            this, &SensorSettings::onAutoDetectionFinished);
    connect(m_autoConnector, &AutoConnector::deviceDetected,
            this, &SensorSettings::onDeviceDetected);
    connect(m_autoConnector, &AutoConnector::progressUpdated,
            this, &SensorSettings::onAutoConnectProgress);
    connect(m_autoConnector, &AutoConnector::logMessage,
            this, &SensorSettings::onAutoConnectLog);

    // Загружаем сохранённые настройки (перезапишут дефолты если есть)
    loadSettings();
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

    // БИНС
    connect(ui->btnRefreshBinsPorts, &QPushButton::clicked, this, &SensorSettings::onRefreshBinsPortsClicked);
    connect(ui->btnConnectBins, &QPushButton::clicked, this, &SensorSettings::onConnectBinsClicked);
    connect(ui->btnDisconnectBins, &QPushButton::clicked, this, &SensorSettings::onDisconnectBinsClicked);

    // Автоподключение
    connect(ui->btnAutoConnect, &QPushButton::clicked, this, &SensorSettings::onAutoConnectClicked);

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
    if (QFile::exists("/dev/pts/1")){
        ui->comboBoxComPort->addItem("Virtual pts/1", "/dev/pts/1");
    }
    if (QFile::exists("/dev/pts/2")){
        ui->comboBoxComPort->addItem("Virtual pts/2", "/dev/pts/2");
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
    if (QFile::exists("/dev/pts/1")){
        ui->comboBoxGnssPort->addItem("Virtual pts/1", "/dev/pts/1");
    }
    if (QFile::exists("/dev/pts/2")){
        ui->comboBoxGnssPort->addItem("Virtual pts/2", "/dev/pts/2");
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

void SensorSettings::populateBinsPorts()
{
    ui->comboBoxBinsPort->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString portInfo = QString("%1 (%2)").arg(info.portName()).arg(info.description());
        ui->comboBoxBinsPort->addItem(portInfo, info.systemLocation());
        qDebug() << "SensorSettings: Найден порт БИНС:" << portInfo << "->" << info.systemLocation();
    }

    // Добавляем виртуальные pts порты
    QDir ptsDir("/dev/pts");
    if (ptsDir.exists()) {
        QStringList ptsFiles = ptsDir.entryList(QDir::System | QDir::Readable | QDir::NoDotAndDotDot);
        for (const QString &ptsFile : ptsFiles) {
            bool isNumber;
            ptsFile.toInt(&isNumber);
            if (isNumber) {
                QString ptsPath = "/dev/pts/" + ptsFile;
                ui->comboBoxBinsPort->addItem(QString("Virtual pts/%1").arg(ptsFile), ptsPath);
                qDebug() << "SensorSettings: Найден виртуальный порт БИНС:" << ptsPath;
            }
        }
    }

    if (ui->comboBoxBinsPort->count() == 0) {
        ui->comboBoxBinsPort->addItem("Нет доступных портов");
        ui->btnConnectBins->setEnabled(false);
    } else {
        ui->btnConnectBins->setEnabled(true);
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

void SensorSettings::onRefreshBinsPortsClicked()
{
    populateBinsPorts();
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
    // Блокируем кнопку подключения и настройки во время процесса
    ui->btnConnectAms->setEnabled(false);
    ui->btnConnectAms->setText("Подключение...");
    setAmsConnectionEnabled(false);

    // Включаем кнопку отключения на случай, если нужно прервать
    ui->btnDisconnectAms->setEnabled(true);
    ui->btnDisconnectAms->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");

    // Обновляем статус
    ui->lblAmsStatus->setText("Подключение...");
    ui->lblAmsStatus->setStyleSheet("color: orange; font-size: 10pt; padding: 5px; font-weight: bold;");

    emit amsConnectRequested();

    // Защитный таймаут: если через 8 секунд статус не изменился —
    // возвращаем кнопки в рабочее состояние самостоятельно
    QTimer::singleShot(8000, this, [this]() {
        // Проверяем: если всё ещё в состоянии "Подключение..." — сбрасываем
        if (ui->lblAmsStatus->text() == "Подключение...") {
            setAmsConnectionStatus("Нет ответа от АМС", false);
            setAmsConnectionEnabled(true);
        }
    });
}

void SensorSettings::onDisconnectAmsClicked()
{
    emit amsDisconnectRequested();
}

void SensorSettings::onCloseClicked()
{
    saveSettings();
    close();
}

void SensorSettings::onConnectBinsClicked()
{
    ui->btnConnectBins->setEnabled(false);
    ui->btnConnectBins->setText("Подключение...");
    setBinsConnectionEnabled(false);

    ui->btnDisconnectBins->setEnabled(true);
    ui->btnDisconnectBins->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");

    ui->lblBinsStatus->setText("Подключение...");
    ui->lblBinsStatus->setStyleSheet("color: orange; font-size: 10pt; padding: 5px; font-weight: bold;");

    emit binsConnectRequested();
}

void SensorSettings::onDisconnectBinsClicked()
{
    emit binsDisconnectRequested();
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

// ===== МЕТОДЫ ДЛЯ GNSS (независимо от активной вкладки) =====

QString SensorSettings::getGnssComPort() const
{
    QString port = ui->comboBoxGnssPort->currentData().toString();
    return port;
}

int SensorSettings::getGnssBaudRate() const
{
    int baudRate = ui->comboBoxGnssBaudRate->currentText().toInt();
    return baudRate;
}

QSerialPort::DataBits SensorSettings::getGnssDataBits() const
{
    int bits = ui->comboBoxGnssDataBits->currentText().toInt();
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getGnssParity() const
{
    int index = ui->comboBoxGnssParity->currentIndex();
    switch (index) {
    case 1: return QSerialPort::EvenParity;
    case 2: return QSerialPort::OddParity;
    default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getGnssStopBits() const
{
    int index = ui->comboBoxGnssStopBits->currentIndex();
    return (index == 0) ? QSerialPort::OneStop : QSerialPort::TwoStop;
}

// ===== МЕТОДЫ ДЛЯ ИВС (независимо от активной вкладки) =====

QString SensorSettings::getIwsComPort() const
{
    QString port = ui->comboBoxComPort->currentData().toString();
    return port;
}

int SensorSettings::getIwsBaudRate() const
{
    int baudRate = ui->comboBoxBaudRate->currentText().toInt();
    return baudRate;
}

QSerialPort::DataBits SensorSettings::getIwsDataBits() const
{
    int bits = ui->comboBoxDataBits->currentText().toInt();
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getIwsParity() const
{
    int index = ui->comboBoxParity->currentIndex();
    switch (index) {
    case 1: return QSerialPort::EvenParity;
    case 2: return QSerialPort::OddParity;
    default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getIwsStopBits() const
{
    int index = ui->comboBoxStopBits->currentIndex();
    return (index == 0) ? QSerialPort::OneStop : QSerialPort::TwoStop;
}

int SensorSettings::getIwsProtocolIndex() const
{
    return ui->comboBoxProtocol->currentIndex();
}

quint8 SensorSettings::getIwsDeviceAddress() const
{
    return static_cast<quint8>(ui->spinBoxDeviceAddress->value());
}

int SensorSettings::getIwsPollInterval() const
{
    return ui->spinBoxPollInterval->value();
}

// ===== МЕТОДЫ ДЛЯ БИНС =====
QString SensorSettings::getBinsComPort() const
{
    return ui->comboBoxBinsPort->currentData().toString();
}

int SensorSettings::getBinsBaudRate() const
{
    return ui->comboBoxBinsBaudRate->currentText().toInt();
}

QSerialPort::DataBits SensorSettings::getBinsDataBits() const
{
    int bits = ui->comboBoxBinsDataBits->currentText().toInt();
    return (bits == 8) ? QSerialPort::Data8 : QSerialPort::Data7;
}

QSerialPort::Parity SensorSettings::getBinsParity() const
{
    int index = ui->comboBoxBinsParity->currentIndex();
    switch (index) {
    case 1: return QSerialPort::EvenParity;
    case 2: return QSerialPort::OddParity;
    default: return QSerialPort::NoParity;
    }
}

QSerialPort::StopBits SensorSettings::getBinsStopBits() const
{
    int index = ui->comboBoxBinsStopBits->currentIndex();
    return (index == 0) ? QSerialPort::OneStop : QSerialPort::TwoStop;
}

void SensorSettings::setBinsConnectionStatus(const QString &status, bool connected)
{
    ui->lblBinsStatus->setText(QString("Статус: %1").arg(status));

    if (connected) {
        ui->lblBinsStatus->setStyleSheet("color: green; font-size: 10pt; padding: 5px; font-weight: bold;");
        ui->btnConnectBins->setEnabled(false);
        ui->btnConnectBins->setText("Подключить"); // Сбрасываем текст
        ui->btnDisconnectBins->setEnabled(true);
        ui->btnDisconnectBins->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
    } else {
        ui->lblBinsStatus->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
        ui->btnConnectBins->setEnabled(true);
        ui->btnConnectBins->setText("Подключить"); // Сбрасываем текст
        ui->btnConnectBins->setStyleSheet(""); // Убираем стиль
        ui->btnDisconnectBins->setEnabled(false);
        ui->btnDisconnectBins->setStyleSheet("background-color: #757575; color: white; font-weight: bold;");
    }

    qDebug() << "SensorSettings: Статус БИНС установлен:" << status << "Connected:" << connected;
}

void SensorSettings::setBinsConnectionEnabled(bool enabled)
{
    ui->comboBoxBinsPort->setEnabled(enabled);
    ui->comboBoxBinsBaudRate->setEnabled(enabled);
    ui->comboBoxBinsDataBits->setEnabled(enabled);
    ui->comboBoxBinsParity->setEnabled(enabled);
    ui->comboBoxBinsStopBits->setEnabled(enabled);
    ui->btnRefreshBinsPorts->setEnabled(enabled);
}

void SensorSettings::setAmsConnectionStatus(const QString& status, bool connected)
{
    ui->lblAmsStatus->setText(QString("Статус: %1").arg(status));

    if (connected) {
        ui->lblAmsStatus->setStyleSheet("color: green; font-size: 10pt; padding: 5px; font-weight: bold;");
        ui->btnConnectAms->setEnabled(false);
        ui->btnConnectAms->setText("Подключить"); // Сбрасываем текст
        ui->btnDisconnectAms->setEnabled(true);
        ui->btnDisconnectAms->setStyleSheet("background-color: #F44336; color: white; font-weight: bold;");
    } else {
        ui->lblAmsStatus->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
        ui->btnConnectAms->setEnabled(true);
        ui->btnConnectAms->setText("Подключить"); // Сбрасываем текст
        ui->btnConnectAms->setStyleSheet(""); // Убираем стиль
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

void SensorSettings::setAmsConnectionError(const QString& errorMessage)
{
    // Устанавливаем статус ошибки
    ui->lblAmsStatus->setText(QString("Ошибка: %1").arg(errorMessage));
    ui->lblAmsStatus->setStyleSheet("color: red; font-size: 10pt; padding: 5px; font-weight: bold;");

    // Возвращаем кнопки в состояние "не подключено"
    ui->btnConnectAms->setEnabled(true);
    ui->btnDisconnectAms->setEnabled(false);
    ui->btnDisconnectAms->setStyleSheet("background-color: #757575; color: white; font-weight: bold;");
    ui->btnConnectAms->setStyleSheet(""); // Убираем стиль

    // Включаем настройки для повторного подключения
    setAmsConnectionEnabled(true);

    qWarning() << "SensorSettings: Ошибка подключения АМС:" << errorMessage;
}

// ===== МЕТОДЫ ДЛЯ GNSS (независимо от вкладки) =====

void SensorSettings::setGnssConnectionStatus(const QString& status, bool connected)
{
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

    qDebug() << "SensorSettings: GNSS статус установлен:" << status << "Connected:" << connected;
}

void SensorSettings::setGnssConnectionEnabled(bool enabled)
{
    ui->comboBoxGnssPort->setEnabled(enabled);
    ui->comboBoxGnssBaudRate->setEnabled(enabled);
    ui->comboBoxGnssDataBits->setEnabled(enabled);
    ui->comboBoxGnssParity->setEnabled(enabled);
    ui->comboBoxGnssStopBits->setEnabled(enabled);
    ui->btnRefreshGnssPorts->setEnabled(enabled);
}

// ===== МЕТОДЫ ДЛЯ ИВС (независимо от вкладки) =====

void SensorSettings::setIwsConnectionStatus(const QString& status, bool connected)
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

    qDebug() << "SensorSettings: ИВС статус установлен:" << status << "Connected:" << connected;
}

void SensorSettings::setIwsConnectionEnabled(bool enabled)
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

// ===== AutoConnector слоты =====

void SensorSettings::onAutoConnectClicked()
{
    qDebug() << "Запуск автоопределения устройств...";

    // Отключаем все кнопки подключения на время автопоиска
    ui->btnConnect->setEnabled(false);
    ui->btnConnectGnss->setEnabled(false);
    ui->btnConnectAms->setEnabled(false);
    ui->btnAutoConnect->setEnabled(false);
    ui->btnAutoConnect->setText("Поиск...");

    // Запускаем автоопределение
    m_autoConnector->startDetection();
}

void SensorSettings::onAutoDetectionStarted()
{
    qDebug() << "Автоопределение началось";
//    ui->labelStatus->setText("Поиск устройств...");
}

void SensorSettings::onAutoDetectionFinished()
{
    qDebug() << "Автоопределение завершено";

    // Включаем кнопки обратно
    ui->btnConnect->setEnabled(true);
    ui->btnConnectGnss->setEnabled(true);
    ui->btnConnectAms->setEnabled(true);
    ui->btnAutoConnect->setEnabled(true);
    ui->btnAutoConnect->setText("Автоподключение");

    // Получаем результаты
    QMap<AutoConnector::DeviceType, AutoConnector::DeviceInfo> devices =
        m_autoConnector->getDetectedDevices();

    if (devices.isEmpty()) {
//        ui->labelStatus->setText("Устройства не найдены");
        qDebug() << "Устройства не найдены на доступных портах";
        return;
    }

    // Автоматически устанавливаем найденные порты в ComboBox
    int foundCount = 0;
    bool hasAms = false;
    bool hasGnss = false;
    bool hasIws = false;

    for (auto it = devices.begin(); it != devices.end(); ++it) {
        AutoConnector::DeviceInfo info = it.value();
        foundCount++;

        switch (it.key()) {
            case AutoConnector::DEVICE_AMS:
                qDebug() << "АМС найден на" << info.portName << "(" << info.baudRate << "бод)";
                setComboBoxPort(ui->comboBoxAmsPort, info.portName);
                ui->comboBoxAmsBaudRate->setCurrentText(QString::number(info.baudRate));
//                ui->labelStatusAms->setText(QString("✓ Найден: %1").arg(info.portName));
                hasAms = true;
                break;

            case AutoConnector::DEVICE_GNSS:
                qDebug() << "GNSS найден на" << info.portName << "(" << info.baudRate << "бод)";
                setComboBoxPort(ui->comboBoxGnssPort, info.portName);
                ui->comboBoxGnssBaudRate->setCurrentText(QString::number(info.baudRate));
//                ui->labelStatusGnss->setText(QString("✓ Найден: %1").arg(info.portName));
                hasGnss = true;
                break;

            case AutoConnector::DEVICE_IWS:
                qDebug() << "ИВС найден на" << info.portName << "(" << info.baudRate << "бод)";
                setComboBoxPort(ui->comboBoxComPort, info.portName);
                ui->comboBoxBaudRate->setCurrentText(QString::number(info.baudRate));
//                ui->labelStatus->setText(QString("✓ Найден: %1").arg(info.portName));
                hasIws = true;
                break;

            default:
                break;
        }
    }

    qDebug() << "Автоопределение завершено. Найдено устройств:" << foundCount;

    // === АВТОМАТИЧЕСКОЕ ПОДКЛЮЧЕНИЕ КО ВСЕМ НАЙДЕННЫМ УСТРОЙСТВАМ ===
    // Используем задержки чтобы подключения не конфликтовали

    if (hasAms) {
        qDebug() << "Автоподключение к АМС...";
        QTimer::singleShot(200, this, [this]() {
            emit amsConnectRequested();
        });
    }

    if (hasGnss) {
        qDebug() << "Автоподключение к GNSS...";
        QTimer::singleShot(400, this, [this]() {
            emit gnssConnectRequested();
        });
    }

    if (hasIws) {
        qDebug() << "Автоподключение к ИВС...";
        QTimer::singleShot(600, this, [this]() {
            emit connectRequested();
        });
    }

    // Сохраняем найденные порты сразу после автопоиска
    saveSettings();
}

void SensorSettings::onDeviceDetected(AutoConnector::DeviceType type,
                                     const QString &portName,
                                     int baudRate)
{
    QString deviceName;
    switch (type) {
        case AutoConnector::DEVICE_AMS:  deviceName = "АМС"; break;
        case AutoConnector::DEVICE_GNSS: deviceName = "GNSS"; break;
        case AutoConnector::DEVICE_IWS:  deviceName = "ИВС"; break;
        default: deviceName = "Неизвестное"; break;
    }

    qDebug() << "Обнаружено:" << deviceName << "на" << portName << baudRate << "бод";
}

void SensorSettings::onAutoConnectProgress(int current, int total)
{
    QString statusText = QString("Проверка порта %1 из %2...").arg(current).arg(total);
//    ui->labelStatus->setText(statusText);
    qDebug() << statusText;
}

void SensorSettings::onAutoConnectLog(const QString &message)
{
    qDebug() << "[AutoConnect]" << message;
}

void SensorSettings::setComboBoxPort(QComboBox *comboBox, const QString &portName)
{
    // Сначала ищем по userData (полный путь /dev/pts/1)
    for (int i = 0; i < comboBox->count(); i++) {
        QString userData = comboBox->itemData(i).toString();
        if (userData == portName) {
            qDebug() << "Порт найден по userData:" << portName << "на позиции" << i;
            comboBox->setCurrentIndex(i);
            return;
        }
    }

    // Если не нашли по userData, ищем по тексту
    int index = comboBox->findText(portName);
    if (index >= 0) {
        qDebug() << "Порт найден по тексту:" << portName << "на позиции" << index;
        comboBox->setCurrentIndex(index);
        return;
    }

    // Если порт не найден вообще - добавляем его
    qDebug() << "Порт" << portName << "не найден в ComboBox, добавляем...";
    comboBox->addItem(portName, portName);  // Текст и userData одинаковые
    comboBox->setCurrentIndex(comboBox->count() - 1);
}

// ===== СОХРАНЕНИЕ / ЗАГРУЗКА НАСТРОЕК =====

void SensorSettings::saveSettings()
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    settings.beginGroup("Sensors");

    // АМС
    settings.setValue("ams/port",     ui->comboBoxAmsPort->currentData().toString());
    settings.setValue("ams/baudrate", ui->comboBoxAmsBaudRate->currentText());
    settings.setValue("ams/databits", ui->comboBoxAmsDataBits->currentText());
    settings.setValue("ams/parity",   ui->comboBoxAmsParity->currentIndex());
    settings.setValue("ams/stopbits", ui->comboBoxAmsStopBits->currentIndex());

    // GNSS
    settings.setValue("gnss/port",     ui->comboBoxGnssPort->currentData().toString());
    settings.setValue("gnss/baudrate", ui->comboBoxGnssBaudRate->currentText());
    settings.setValue("gnss/databits", ui->comboBoxGnssDataBits->currentText());
    settings.setValue("gnss/parity",   ui->comboBoxGnssParity->currentIndex());
    settings.setValue("gnss/stopbits", ui->comboBoxGnssStopBits->currentIndex());

    // ИВС
    settings.setValue("iws/port",        ui->comboBoxComPort->currentData().toString());
    settings.setValue("iws/baudrate",    ui->comboBoxBaudRate->currentText());
    settings.setValue("iws/databits",    ui->comboBoxDataBits->currentText());
    settings.setValue("iws/parity",      ui->comboBoxParity->currentIndex());
    settings.setValue("iws/stopbits",    ui->comboBoxStopBits->currentIndex());
    settings.setValue("iws/protocol",    ui->comboBoxProtocol->currentIndex());
    settings.setValue("iws/address",     ui->spinBoxDeviceAddress->value());
    settings.setValue("iws/pollinterval",ui->spinBoxPollInterval->value());

    // БИНС
    settings.setValue("bins/port",     ui->comboBoxBinsPort->currentData().toString());
    settings.setValue("bins/baudrate", ui->comboBoxBinsBaudRate->currentText());
    settings.setValue("bins/databits", ui->comboBoxBinsDataBits->currentText());
    settings.setValue("bins/parity",   ui->comboBoxBinsParity->currentIndex());
    settings.setValue("bins/stopbits", ui->comboBoxBinsStopBits->currentIndex());

    settings.endGroup();

    qDebug() << "SensorSettings: Настройки сохранены в" << settings.fileName();
}

void SensorSettings::loadSettings()
{
    QSettings settings(QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    // Если настроек ещё не было — выходим, оставляем дефолты
    if (!settings.childGroups().contains("Sensors")) {
        qDebug() << "SensorSettings: Сохранённых настроек нет, используются значения по умолчанию";
        return;
    }

    settings.beginGroup("Sensors");

    // АМС
    QString amsPort = settings.value("ams/port").toString();
    if (!amsPort.isEmpty()) {
        setComboBoxPort(ui->comboBoxAmsPort, amsPort);
    }
    if (settings.contains("ams/baudrate"))
        ui->comboBoxAmsBaudRate->setCurrentText(settings.value("ams/baudrate").toString());
    if (settings.contains("ams/databits"))
        ui->comboBoxAmsDataBits->setCurrentText(settings.value("ams/databits").toString());
    if (settings.contains("ams/parity"))
        ui->comboBoxAmsParity->setCurrentIndex(settings.value("ams/parity").toInt());
    if (settings.contains("ams/stopbits"))
        ui->comboBoxAmsStopBits->setCurrentIndex(settings.value("ams/stopbits").toInt());

    // GNSS
    QString gnssPort = settings.value("gnss/port").toString();
    if (!gnssPort.isEmpty()) {
        setComboBoxPort(ui->comboBoxGnssPort, gnssPort);
    }
    if (settings.contains("gnss/baudrate"))
        ui->comboBoxGnssBaudRate->setCurrentText(settings.value("gnss/baudrate").toString());
    if (settings.contains("gnss/databits"))
        ui->comboBoxGnssDataBits->setCurrentText(settings.value("gnss/databits").toString());
    if (settings.contains("gnss/parity"))
        ui->comboBoxGnssParity->setCurrentIndex(settings.value("gnss/parity").toInt());
    if (settings.contains("gnss/stopbits"))
        ui->comboBoxGnssStopBits->setCurrentIndex(settings.value("gnss/stopbits").toInt());

    // ИВС
    QString iwsPort = settings.value("iws/port").toString();
    if (!iwsPort.isEmpty()) {
        setComboBoxPort(ui->comboBoxComPort, iwsPort);
    }
    if (settings.contains("iws/baudrate"))
        ui->comboBoxBaudRate->setCurrentText(settings.value("iws/baudrate").toString());
    if (settings.contains("iws/databits"))
        ui->comboBoxDataBits->setCurrentText(settings.value("iws/databits").toString());
    if (settings.contains("iws/parity"))
        ui->comboBoxParity->setCurrentIndex(settings.value("iws/parity").toInt());
    if (settings.contains("iws/stopbits"))
        ui->comboBoxStopBits->setCurrentIndex(settings.value("iws/stopbits").toInt());
    if (settings.contains("iws/protocol"))
        ui->comboBoxProtocol->setCurrentIndex(settings.value("iws/protocol").toInt());
    if (settings.contains("iws/address"))
        ui->spinBoxDeviceAddress->setValue(settings.value("iws/address").toInt());
    if (settings.contains("iws/pollinterval"))
        ui->spinBoxPollInterval->setValue(settings.value("iws/pollinterval").toInt());

    // БИНС
    QString binsPort = settings.value("bins/port").toString();
    if (!binsPort.isEmpty()) {
        setComboBoxPort(ui->comboBoxBinsPort, binsPort);
    }
    if (settings.contains("bins/baudrate"))
        ui->comboBoxBinsBaudRate->setCurrentText(settings.value("bins/baudrate").toString());
    if (settings.contains("bins/databits"))
        ui->comboBoxBinsDataBits->setCurrentText(settings.value("bins/databits").toString());
    if (settings.contains("bins/parity"))
        ui->comboBoxBinsParity->setCurrentIndex(settings.value("bins/parity").toInt());
    if (settings.contains("bins/stopbits"))
        ui->comboBoxBinsStopBits->setCurrentIndex(settings.value("bins/stopbits").toInt());

    settings.endGroup();

    qDebug() << "SensorSettings: Настройки загружены из" << settings.fileName();
}
