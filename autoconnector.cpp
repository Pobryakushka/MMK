#include "autoconnector.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QtEndian>
#include <QDir>
#include "amsprotocol.h"

//TODO Сделать поиск для каждой системы со своей скоростью. Если система была найдена убрать систему из поиска.

AutoConnector::AutoConnector(QObject *parent)
    : QObject(parent)
    , m_currentPortIndex(0)
    , m_testPort(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_currentPhase(PHASE_AMS_TEST)
    , m_isDetecting(false)
    , m_currentBaudRate(0)
    , m_deviceFoundOnCurrentPort(false)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &AutoConnector::onTimeout);
}

AutoConnector::~AutoConnector()
{
    stopDetection();
}

static QStringList collectAllSerialLikePorts()
{
    QStringList result;

    // --- Реальные serial порты (Qt / udev) ---
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        QString path = info.systemLocation(); // /dev/ttyS0 и т.п.
        if (!path.isEmpty() && !result.contains(path))
            result.append(path);
    }

    // --- PTY (socat, эмуляторы) ---
#ifdef Q_OS_LINUX
    QDir ptsDir("/dev/pts");
    if (ptsDir.exists()) {
        const QStringList entries =
            ptsDir.entryList(QDir::System | QDir::Readable | QDir::NoDotAndDotDot);

        for (const QString &e : entries) {
            bool ok = false;
            e.toInt(&ok);
            if (!ok)
                continue; // ptmx и прочее

            QString ptsPath = "/dev/pts/" + e;
            if (!result.contains(ptsPath))
                result.append(ptsPath);
        }
    }
#endif

    return result;
}

void AutoConnector::startDetection()
{
    if (m_isDetecting) {
        emit logMessage("Автоопределение уже запущено");
        return;
    }

    emit logMessage("=== НАЧАЛО АВТООПРЕДЕЛЕНИЯ УСТРОЙСТВ ===");

    // Очищаем предыдущие результаты
    m_detectedDevices.clear();
    m_portsToCheck.clear();
    m_currentPortIndex = 0;
    m_isDetecting = true;

    // Получаем список доступных портов (реальные + /dev/pts)
    QStringList ports = collectAllSerialLikePorts();

    for (const QString &portPath : ports) {

        // Пропускаем Bluetooth (на всякий случай)
        if (portPath.contains("Bluetooth", Qt::CaseInsensitive))
            continue;

        m_portsToCheck.append(portPath);

        emit logMessage(QString("Найден порт: %1")
                       .arg(portPath));
    }


    if (m_portsToCheck.isEmpty()) {
        emit logMessage("Не найдено доступных портов");
        m_isDetecting = false;
        emit detectionFinished();
        return;
    }

    emit logMessage(QString("Всего портов для проверки: %1").arg(m_portsToCheck.size()));
    emit detectionStarted();

    // Начинаем с первого порта
    processNextPort();
}

void AutoConnector::stopDetection()
{
    if (!m_isDetecting) {
        return;
    }

    closeCurrentPort();
    m_timeoutTimer->stop();
    m_isDetecting = false;

    emit logMessage("Автоопределение остановлено");
}

// Возвращает правильную скорость для каждого типа устройства
int AutoConnector::baudRateForPhase(TestPhase phase)
{
    switch (phase) {
        case PHASE_AMS_TEST:    return 115200; // АМС: 115200
        case PHASE_GNSS_LISTEN: return 9600;  // GNSS: 19200
        case PHASE_IWS_TEST:    return 19200;  // ИВС: 19200
        default:                return 9600;
    }
}

void AutoConnector::processNextPort()
{
    // Закрываем предыдущий порт если был открыт
    closeCurrentPort();

    // Проверяем есть ли еще порты
    if (m_currentPortIndex >= m_portsToCheck.size()) {
        // Все порты проверены
        m_isDetecting = false;
        emit logMessage("=== АВТООПРЕДЕЛЕНИЕ ЗАВЕРШЕНО ===");
        emit logMessage(QString("Найдено устройств: %1").arg(m_detectedDevices.size()));
        emit detectionFinished();
        return;
    }

    m_currentPortName = m_portsToCheck[m_currentPortIndex];
    m_deviceFoundOnCurrentPort = false;

    // Определяем с какой фазы начинать — пропускаем уже найденные типы
    m_currentPhase = PHASE_DONE; // будет перезаписано ниже

    if (!m_detectedDevices.contains(DEVICE_AMS))
        m_currentPhase = PHASE_AMS_TEST;
    else if (!m_detectedDevices.contains(DEVICE_GNSS))
        m_currentPhase = PHASE_GNSS_LISTEN;
    else if (!m_detectedDevices.contains(DEVICE_IWS))
        m_currentPhase = PHASE_IWS_TEST;

    // Все устройства уже найдены — останавливаемся
    if (m_currentPhase == PHASE_DONE) {
        m_isDetecting = false;
        emit logMessage("=== ВСЕ УСТРОЙСТВА НАЙДЕНЫ, ПОИСК ЗАВЕРШЁН ===");
        emit detectionFinished();
        return;
    }

    emit progressUpdated(m_currentPortIndex + 1, m_portsToCheck.size());
    emit logMessage(QString("\n--- Проверка порта %1 (%2/%3) ---")
                   .arg(m_currentPortName)
                   .arg(m_currentPortIndex + 1)
                   .arg(m_portsToCheck.size()));

    // Скорость выбирается под первую фазу которую будем тестировать
    int startBaudRate = baudRateForPhase(m_currentPhase);
    openPortAndTest(m_currentPortName, startBaudRate);
}

void AutoConnector::openPortAndTest(const QString &portName, int baudRate)
{
    m_currentBaudRate = baudRate;

    // Создаем новый порт
    m_testPort = new QSerialPort(this);
    m_testPort->setPortName(portName);
    m_testPort->setBaudRate(baudRate);
    m_testPort->setDataBits(QSerialPort::Data8);
    m_testPort->setParity(QSerialPort::NoParity);
    m_testPort->setStopBits(QSerialPort::OneStop);

    connect(m_testPort, &QSerialPort::readyRead, this, &AutoConnector::onReadyRead);

    if (!m_testPort->open(QIODevice::ReadWrite)) {
        emit logMessage(QString("  Ошибка открытия %1 (%2): %3")
                       .arg(portName)
                       .arg(baudRate)
                       .arg(m_testPort->errorString()));
        m_testPort->deleteLater();
        m_testPort = nullptr;

        // Переходим к следующему порту
        m_currentPortIndex++;
        QTimer::singleShot(100, this, &AutoConnector::processNextPort);
        return;
    }

    emit logMessage(QString("  Порт открыт: %1 бод").arg(baudRate));

    m_receiveBuffer.clear();

    // Запускаем тест для текущей фазы
    switch (m_currentPhase) {
        case PHASE_AMS_TEST:    testAMS();  break;
        case PHASE_GNSS_LISTEN: testGNSS(); break;
        case PHASE_IWS_TEST:    testIWS();  break;
        default: moveToNextPhase(); break;
    }
}

void AutoConnector::closeCurrentPort()
{
    if (m_testPort) {
        if (m_testPort->isOpen()) {
            m_testPort->close();
        }
        m_testPort->deleteLater();
        m_testPort = nullptr;
    }
    m_receiveBuffer.clear();
}

void AutoConnector::testAMS()
{
    emit logMessage("  Тест АМС: отправка LINE_TEST...");

    QByteArray command = createAmsLineTestCommand();

    if (m_testPort && m_testPort->isOpen()) {
        m_testPort->write(command);
        m_testPort->flush();

        // Ждем ответ 1 секунду
        m_timeoutTimer->start(5000);
    } else {
        moveToNextPhase();
    }
}

void AutoConnector::testGNSS()
{
    emit logMessage("  Тест GNSS: слушаем NMEA данные...");

    // Для GNSS просто слушаем порт 2 секунды
    // GNSS сам отправляет данные
    m_timeoutTimer->start(2000);
}

void AutoConnector::testIWS()
{
    emit logMessage("  Тест ИВС (UMB): отправка запроса...");

    QByteArray command = createIwsTestCommand();

    if (m_testPort && m_testPort->isOpen()) {
        m_testPort->write(command);
        m_testPort->flush();

        // Ждем ответ 1 секунду
        m_timeoutTimer->start(1000);
    } else {
        moveToNextPhase();
    }
}

void AutoConnector::onReadyRead()
{
    if (!m_testPort) return;

    QByteArray data = m_testPort->readAll();
    m_receiveBuffer.append(data);

    emit logMessage(QString("  Получено %1 байт: %2")
                   .arg(data.size())
                   .arg(QString(data.toHex(' '))));

    // Проверяем тип устройства в зависимости от текущей фазы
    switch (m_currentPhase) {
        case PHASE_AMS_TEST:
            if (isAmsResponse(m_receiveBuffer)) {
                m_timeoutTimer->stop();
                deviceFound(DEVICE_AMS, "АМС (Акустический Метеопост)");
                return;
            }
            break;

        case PHASE_GNSS_LISTEN:
            if (isNmeaData(m_receiveBuffer)) {
                m_timeoutTimer->stop();
                deviceFound(DEVICE_GNSS, "GNSS приемник");
                return;
            }
            break;

        case PHASE_IWS_TEST:
            if (isUmbResponse(m_receiveBuffer)) {
                m_timeoutTimer->stop();
                deviceFound(DEVICE_IWS, "ИВС (UMB протокол)");
                return;
            }
            break;

        case PHASE_DONE:
            break;
    }
}

void AutoConnector::onTimeout()
{
    emit logMessage("  Таймаут - устройство не отвечает");
    moveToNextPhase();
}

void AutoConnector::moveToNextPhase()
{
    if (m_deviceFoundOnCurrentPort) {
        // Устройство уже найдено на этом порту, переходим к следующему
        m_currentPortIndex++;
        processNextPort();
        return;
    }

    // Переходим к следующей незайденной фазе
    TestPhase nextPhase = PHASE_DONE;

    if (m_currentPhase == PHASE_AMS_TEST) {
        // Ищем следующую фазу после AMS
        if (!m_detectedDevices.contains(DEVICE_GNSS))
            nextPhase = PHASE_GNSS_LISTEN;
        else if (!m_detectedDevices.contains(DEVICE_IWS))
            nextPhase = PHASE_IWS_TEST;
    } else if (m_currentPhase == PHASE_GNSS_LISTEN) {
        if (!m_detectedDevices.contains(DEVICE_IWS))
            nextPhase = PHASE_IWS_TEST;
    }
    // PHASE_IWS_TEST и PHASE_DONE → всегда переходим к следующему порту

    if (nextPhase == PHASE_DONE) {
        // Все нужные фазы на этом порту пройдены — следующий порт
        m_currentPortIndex++;
        processNextPort();
        return;
    }

    // Переключаем фазу и переоткрываем порт с нужной скоростью
    m_currentPhase = nextPhase;
    m_receiveBuffer.clear();

    int newBaudRate = baudRateForPhase(nextPhase);

    // Если скорость не изменилась — не нужно переоткрывать порт
    if (newBaudRate == m_currentBaudRate) {
        switch (nextPhase) {
            case PHASE_GNSS_LISTEN: testGNSS(); break;
            case PHASE_IWS_TEST:    testIWS();  break;
            default: break;
        }
    } else {
        // Переоткрываем порт с новой скоростью
        closeCurrentPort();
        emit logMessage(QString("  Смена скорости: %1 → %2 бод").arg(m_currentBaudRate).arg(newBaudRate));
        openPortAndTest(m_currentPortName, newBaudRate);
    }
}

void AutoConnector::deviceFound(DeviceType type, const QString &description)
{
    emit logMessage(QString("  ✓ НАЙДЕНО: %1").arg(description));

    DeviceInfo info;
    info.type = type;
    info.portName = m_currentPortName;
    info.baudRate = m_currentBaudRate;
    info.description = description;

    m_detectedDevices[type] = info;
    m_deviceFoundOnCurrentPort = true;

    emit deviceDetected(type, m_currentPortName, m_currentBaudRate);

    // Переходим к следующему порту
    m_currentPortIndex++;
    QTimer::singleShot(100, this, &AutoConnector::processNextPort);
}

// ===== ПРОВЕРКИ ОТВЕТОВ =====

bool AutoConnector::isAmsResponse(const QByteArray &data)
{
    // АМС ответ на LINE_TEST: [0xA0] [len] [data...] [crc] [0xFF]
    if (data.size() < 5) return false;

    // Проверяем что начинается с 0xA0 (LINE_TEST)
    if (static_cast<quint8>(data[0]) != 0xA0) return false;

    // Проверяем что заканчивается на 0xFF (стоп-байт)
    if (static_cast<quint8>(data.back()) != 0xFF) return false;

    emit logMessage("  → Обнаружен ответ АМС (команда 0xA0, стоп-байт 0xFF)");
    return true;
}

bool AutoConnector::isNmeaData(const QByteArray &data)
{
    // NMEA строки начинаются с '$' и содержат буквы
    QString str = QString::fromLatin1(data);

    // Ищем NMEA строки типа $GNGGA, $GNRMC, $GPGGA и т.д.
    if (str.contains("$GN") || str.contains("$GP")) {
        emit logMessage("  → Обнаружены NMEA данные");
        return true;
    }

    return false;
}

bool AutoConnector::isUmbResponse(const QByteArray &data)
{
    // UMB ответ: [SOH=0x01] [Ver] [TO] [FROM] [Len] ... [ETX=0x03] [CRC] [EOT=0x04]
    if (data.size() < 10) return false;

    // Проверяем структуру UMB
    if (static_cast<quint8>(data[0]) != 0x01) return false;  // SOH
    if (static_cast<quint8>(data[1]) != 0x10) return false;  // Version
    if (static_cast<quint8>(data.back()) != 0x04) return false;  // EOT

    // Ищем ETX (0x03) перед EOT
    bool hasEtx = false;
    for (int i = data.size() - 4; i > 5 && i < data.size() - 1; i++) {
        if (static_cast<quint8>(data[i]) == 0x03) {
            hasEtx = true;
            break;
        }
    }

    if (hasEtx) {
        emit logMessage("  → Обнаружен ответ UMB (SOH, ETX, EOT)");
        return true;
    }

    return false;
}

// ===== СОЗДАНИЕ КОМАНД =====

QByteArray AutoConnector::createAmsLineTestCommand()
{
    // Команда LINE_TEST: [0xA0] [0x00] [CRC_L] [CRC_H] [0xFF]
//    QByteArray command;
//    command.append(static_cast<char>(0xA0));  // Команда
//    command.append(static_cast<char>(0x00));  // Длина данных = 0

//    // Вычисляем CRC
//    quint16 crc = calculateAmsCrc(command);
//    command.append(static_cast<char>(crc & 0xFF));
//    command.append(static_cast<char>((crc >> 8) & 0xFF));

//    command.append(static_cast<char>(0xFF));  // Стоп-байт

//    qDebug() << "Packet: " << command;
//    return command;

    QByteArray command;
    command.append(static_cast<char>(CMD_LINE_TEST));
    command.append(QByteArray::fromHex("FFFFFFFF"));
    command.append(QByteArray::fromHex("EEEEEEEE"));
    command.append(QByteArray::fromHex("DDDDDDDD"));
    command.append(QByteArray::fromHex("CCCCCC00"));
    return finalizePacketAuto(command);
}

QByteArray AutoConnector::createIwsTestCommand()
{
    // Простой UMB запрос на чтение температуры (0x0064)
    QByteArray command;

    command.append(static_cast<char>(0x01)); // SOH
    command.append(static_cast<char>(0x10)); // ver
    command.append(static_cast<char>(0x01)); // to (high)
    command.append(static_cast<char>(0x70)); // to (low) - device address
    command.append(static_cast<char>(0x01)); // from (high)
    command.append(static_cast<char>(0xF0)); // from (low)
    command.append(static_cast<char>(0x05)); // data length
    command.append(static_cast<char>(0x02)); // STX
    command.append(static_cast<char>(0x2F)); // cmd (read)
    command.append(static_cast<char>(0x10)); // verc
    command.append(static_cast<char>(0x01)); // param count
    command.append(static_cast<char>(0x64)); // param code (low) - температура
    command.append(static_cast<char>(0x00)); // param code (high)
    command.append(static_cast<char>(0x03)); // ETX

    // CRC
    quint16 crc = calculateUmbCrc(command);
    command.append(static_cast<char>(crc & 0xFF));
    command.append(static_cast<char>((crc >> 8) & 0xFF));

    command.append(static_cast<char>(0x04)); // EOT

    return command;
}

// ===== CRC АЛГОРИТМЫ =====

quint8 AutoConnector::calculateChecksumAuto(const QByteArray &data)
{
    // Контрольная сумма = сумма по модулю 2 всех байтов кроме номера команды
    quint8 checksum = 0;
    for (int i = 0; i < data.size(); i++) {
        checksum ^= static_cast<quint8>(data[i]);
    }
    return checksum;
}

QByteArray AutoConnector::finalizePacketAuto(const QByteArray &data)
{
    QByteArray command = data;
    command.append(calculateChecksumAuto(data));
    command.append(static_cast<char>(0xFF)); // Стоповый байт
    qDebug() << "Final packet: " << command;
    return command;
}

quint16 AutoConnector::calculateUmbCrc(const QByteArray &data)
{
    // CRC-16 (полином 0x8408) - как в UMB
    quint16 crc = 0xFFFF;

    for (int i = 0; i < data.size(); i++) {
        quint8 byte = static_cast<quint8>(data[i]);

        for (int j = 0; j < 8; j++) {
            quint16 x16;
            if ((crc & 0x0001) ^ (byte & 0x01)) {
                x16 = 0x8408;
            } else {
                x16 = 0x0000;
            }
            crc = crc >> 1;
            crc ^= x16;
            byte = byte >> 1;
        }
    }

    return crc;
}

QMap<AutoConnector::DeviceType, AutoConnector::DeviceInfo> AutoConnector::getDetectedDevices() const
{
    return m_detectedDevices;
}
