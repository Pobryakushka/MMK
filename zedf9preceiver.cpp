#include "zedf9preceiver.h"
#include <QDebug>
#include <QtMath>
#include <QThread>

// UBX Protocol константы
#define UBX_SYNC1 0xB5
#define UBX_SYNC2 0x62

// UBX Message классы
#define UBX_CLASS_NAV 0x01
#define UBX_CLASS_RXM 0x02
#define UBX_CLASS_INF 0x04
#define UBX_CLASS_ACK 0x05
#define UBX_CLASS_CFG 0x06
#define UBX_CLASS_MON 0x0A
#define UBX_CLASS_AID 0x0B
#define UBX_CLASS_TIM 0x0D
#define UBX_CLASS_ESF 0x10

// UBX NAV сообщения
#define UBX_NAV_PVT 0x07
#define UBX_NAV_HPPOSLLH 0x14
#define UBX_NAV_STATUS 0x03
#define UBX_NAV_DOP 0x04
#define UBX_NAV_SAT 0x35

// UBX CFG сообщения
#define UBX_CFG_PRT 0x00
#define UBX_CFG_MSG 0x01
#define UBX_CFG_RATE 0x08
#define UBX_CFG_NAV5 0x24
#define UBX_CFG_TMODE3 0x71
#define UBX_CFG_CFG 0x09

ZedF9PReceiver::ZedF9PReceiver(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_outputPort(new QSerialPort(this))
    , m_customProtocol(new CustomProtocol(this))
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &ZedF9PReceiver::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &ZedF9PReceiver::onErrorOccurred);

    connect(m_outputPort, &QSerialPort::readyRead, this, &ZedF9PReceiver::onOutputPortReadyRead);
    connect(m_outputPort, &QSerialPort::errorOccurred, this, &ZedF9PReceiver::onOutputPortErrorOccurred);

    connect(m_customProtocol, &CustomProtocol::parseError, this, [this](const QString &error){
        qDebug() << "Custom protocol error:" << error;
    });
}

ZedF9PReceiver::~ZedF9PReceiver()
{
    disconnectFromReceiver();
    disconnectOutputPort();
}

bool ZedF9PReceiver::connectToReceiver(const QString &portName, qint32 baudRate)
{
    if (m_serialPort->isOpen()) {
        disconnectFromReceiver();
    }

    m_buffer.clear();
    m_ubxBuffer.clear();
    m_rtcmBuffer.clear();

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        emit errorOccurred("Не удалось открыть порт: " + m_serialPort->errorString());
        return false;
    }

    m_serialPort->clear(QSerialPort::AllDirections);

    disconnect(m_serialPort, nullptr, this, nullptr);
    connect(m_serialPort, &QSerialPort::readyRead, this, &ZedF9PReceiver::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &ZedF9PReceiver::onErrorOccurred);

    // Устройство ещё не подтверждено — ждём первых данных
    m_confirmed = false;

    // Запускаем таймаут: если за 5 с нет данных — порт закрываем
    if (!m_connectTimer) {
        m_connectTimer = new QTimer(this);
        m_connectTimer->setSingleShot(true);
        connect(m_connectTimer, &QTimer::timeout, this, [this]() {
            if (!m_confirmed && m_serialPort->isOpen()) {
                qWarning() << "ZedF9PReceiver: таймаут — GNSS не отвечает, закрываем порт";
                disconnectFromReceiver();
                emit errorOccurred("GNSS устройство не отвечает");
            }
        });
    }
    m_connectTimer->start(5000);

    // Конфигурируем приёмник (ответ на конфигурацию сам подтвердит устройство)
    QTimer::singleShot(500, this, [this]() {
        configureUBXProtocol();
        enableUBXMessages(true);
        setNavigationRate(1000);
    });

    // connected() будет эмитирован только после получения первых данных
    return true;
}

void ZedF9PReceiver::disconnectFromReceiver()
{
    if (m_connectTimer) m_connectTimer->stop();
    m_confirmed = false;

    if (m_serialPort->isOpen()) {
        disconnect(m_serialPort, &QSerialPort::readyRead, this, &ZedF9PReceiver::onReadyRead);
        disconnect(m_serialPort, &QSerialPort::errorOccurred, this, &ZedF9PReceiver::onErrorOccurred);

        m_serialPort->clear(QSerialPort::AllDirections);

        m_serialPort->close();

        emit disconnected();
    }

    m_buffer.clear();
    m_ubxBuffer.clear();
    m_rtcmBuffer.clear();
}

void ZedF9PReceiver::confirmConnection()
{
    if (m_confirmed) return;
    m_confirmed = true;
    if (m_connectTimer) m_connectTimer->stop();
    qDebug() << "ZedF9PReceiver: устройство подтверждено (получены данные)";
    emit connected();
}

bool ZedF9PReceiver::connectOutputPort(const QString &portName, qint32 baudRate){
    if (m_outputPort->isOpen()){
        disconnectOutputPort();
    }

    m_outputPort->setPortName(portName);
    m_outputPort->setBaudRate(baudRate);
    m_outputPort->setDataBits(QSerialPort::Data8);
    m_outputPort->setParity(QSerialPort::NoParity);
    m_outputPort->setStopBits(QSerialPort::OneStop);
    m_outputPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_outputPort->open(QIODevice::ReadWrite)){
        emit errorOccurred("Не удалось открыть выходной порт: " + m_outputPort->errorString());
        return false;
    }

    emit outputPortConnected();
    return true;
}

void ZedF9PReceiver::disconnectOutputPort(){
    if (m_outputPort->isOpen()){
        m_outputPort->close();
        emit outputPortDisconnected();
    }
    m_outputBuffer.clear();
}

bool ZedF9PReceiver::isOutputPortConnected() const {
    return m_outputPort->isOpen();
}

void ZedF9PReceiver::onOutputPortReadyRead(){
    QByteArray newData = m_outputPort->readAll();
    m_outputBuffer.append(newData);

    m_customProtocol->addData(newData);
}

void ZedF9PReceiver::onOutputPortErrorOccurred(QSerialPort::SerialPortError error){
    if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError){
        emit errorOccurred("Ошибка выходного порта: " + m_outputPort->errorString());
    }
}

bool ZedF9PReceiver::isConnected() const
{
    return m_serialPort->isOpen() && m_confirmed;
}

QStringList ZedF9PReceiver::getAvailablePorts()
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << info.portName();
    }
    return ports;
}

void ZedF9PReceiver::onReadyRead()
{
    QByteArray newData = m_serialPort->readAll();
    m_buffer.append(newData);

    m_customProtocol->addData(newData);

    while (!m_buffer.isEmpty()) {
        // Проверяем на UBX сообщение
        if (m_buffer.size() >= 2 &&
            (quint8)m_buffer[0] == UBX_SYNC1 &&
            (quint8)m_buffer[1] == UBX_SYNC2) {

            if (m_buffer.size() < 8) break; // Ждём минимум заголовка

            quint16 payloadLength = ((quint8)m_buffer[5] << 8) | (quint8)m_buffer[4];
            quint16 totalLength = 8 + payloadLength; // Sync(2) + Class(1) + ID(1) + Len(2) + Payload + CK(2)

            if (m_buffer.size() < totalLength) break;

            QByteArray ubxMsg = m_buffer.left(totalLength);
            m_buffer.remove(0, totalLength);
            confirmConnection(); // UBX пакет — устройство точно отвечает
            parseUBX(ubxMsg);
            continue;
        }

        // Проверяем на RTCM3 сообщение
        if (m_buffer.size() >= 3 && (quint8)m_buffer[0] == 0xD3) {
            quint16 length = (((quint8)m_buffer[1] & 0x03) << 8) | (quint8)m_buffer[2];
            quint16 totalLength = length + 6; // Преамбула(3) + данные + CRC(3)

            if (m_buffer.size() < totalLength) break;

            QByteArray rtcmMsg = m_buffer.left(totalLength);
            m_buffer.remove(0, totalLength);
            parseRTCM3(rtcmMsg);
            continue;
        }

        // Ищем NMEA сообщение
        int startIdx = m_buffer.indexOf('$');
        if (startIdx == -1) {
            m_buffer.clear();
            break;
        }

        if (startIdx > 0) {
            m_buffer.remove(0, startIdx);
        }

        int endIdx = m_buffer.indexOf("\r\n");
        if (endIdx == -1) break;

        QString sentence = QString::fromLatin1(m_buffer.left(endIdx));
        m_buffer.remove(0, endIdx + 2);

        if (validateNMEAChecksum(sentence)) {
            confirmConnection(); // Валидная NMEA — устройство точно отвечает
            parseNMEA(sentence);
            m_lastNMEA = sentence;
            emit nmeaReceived(sentence);
        }
    }
}

bool ZedF9PReceiver::sendCustomData(const CustomNavigationData &data){
    QSerialPort *portToUse = m_outputPort->isOpen() ? m_outputPort : m_serialPort;

    if (!portToUse->isOpen()){
        return false;
    }

    QByteArray packet = m_customProtocol->buildDataPacket(data);
    qint64 written = portToUse->write(packet);
    portToUse->flush();

    return written == packet.size();
}

void ZedF9PReceiver::parseNMEA(const QString &sentence)
{
    QStringList parts = sentence.split(',');
    if (parts.isEmpty()) return;

    QString msgType = parts[0].mid(1, 5); // Убираем '$' и берём 5 символов

    if (msgType.endsWith("GGA")) {
        parseGGA(parts);
    } else if (msgType.endsWith("RMC")) {
        parseRMC(parts);
    } else if (msgType.endsWith("GSA")) {
        parseGSA(parts);
    } else if (msgType.endsWith("VTG")) {
        parseVTG(parts);
    }
}

void ZedF9PReceiver::parseGGA(const QStringList &fields)
{
    if (fields.size() < 15) return;

    // Время
    QString timeStr = fields[1];
    if (timeStr.length() >= 6) {
        QTime time(timeStr.mid(0,2).toInt(), timeStr.mid(2,2).toInt(), timeStr.mid(4,2).toInt());
        m_currentData.timestamp.setTime(time);
    }

    // Координаты
    if (!fields[2].isEmpty() && !fields[4].isEmpty()) {
        m_currentData.latitude = nmeaToDecimal(fields[2], fields[3]);
        m_currentData.longitude = nmeaToDecimal(fields[4], fields[5]);
    }

    // Качество фикса
    m_currentData.fixQuality = fields[6].toInt();

    // RTK статус
    if (m_currentData.fixQuality == 4) {
        m_currentData.rtkFixed = true;
        m_currentData.rtkFloat = false;
        m_currentData.fixType = "RTK Fixed";
    } else if (m_currentData.fixQuality == 5) {
        m_currentData.rtkFloat = true;
        m_currentData.rtkFixed = false;
        m_currentData.fixType = "RTK Float";
    } else if (m_currentData.fixQuality == 1) {
        m_currentData.fixType = "GPS Fix";
        m_currentData.rtkFixed = false;
        m_currentData.rtkFloat = false;
    } else if (m_currentData.fixQuality == 2) {
        m_currentData.fixType = "DGPS";
        m_currentData.rtkFixed = false;
        m_currentData.rtkFloat = false;
    } else {
        m_currentData.fixType = "No Fix";
        m_currentData.rtkFixed = false;
        m_currentData.rtkFloat = false;
    }

    // Спутники
    m_currentData.satellites = fields[7].toInt();

    // HDOP
    m_currentData.hdop = fields[8].toDouble();

    // Высота
    m_currentData.altitude = fields[9].toDouble();

    emit dataReceived(m_currentData);
}

void ZedF9PReceiver::parseRMC(const QStringList &fields)
{
    if (fields.size() < 12) return;

    // Скорость (узлы -> км/ч)
    if (!fields[7].isEmpty()) {
        m_currentData.speed = fields[7].toDouble() * 1.852;
    }

    // Курс
    if (!fields[8].isEmpty()) {
        m_currentData.heading = fields[8].toDouble();
    }

    // Дата
    QString dateStr = fields[9];
    if (dateStr.length() == 6) {
        int day = dateStr.mid(0,2).toInt();
        int month = dateStr.mid(2,2).toInt();
        int year = 2000 + dateStr.mid(4,2).toInt();
        m_currentData.timestamp.setDate(QDate(year, month, day));
    }
}

void ZedF9PReceiver::parseGSA(const QStringList &fields)
{
    // Дополнительная информация о DOP
    if (fields.size() < 18) return;
}

void ZedF9PReceiver::parseVTG(const QStringList &fields)
{
    if (fields.size() < 10) return;

    // Альтернативное чтение скорости в км/ч
    if (!fields[7].isEmpty()) {
        m_currentData.speed = fields[7].toDouble();
    }
}

void ZedF9PReceiver::parseUBX(const QByteArray &data)
{
    if (data.size() < 8) return;

    quint8 msgClass = data[2];
    quint8 msgID = data[3];
    quint16 payloadLength = ((quint8)data[5] << 8) | (quint8)data[4];

    QByteArray payload = data.mid(6, payloadLength);

    UBXMessage msg;
    msg.msgClass = msgClass;
    msg.msgID = msgID;
    msg.payload = payload;

    // Обработка специфичных сообщений
    if (msgClass == UBX_CLASS_NAV && msgID == UBX_NAV_PVT) {
        parseUBX_NAV_PVT(payload);
    } else if (msgClass == UBX_CLASS_NAV && msgID == UBX_NAV_HPPOSLLH) {
        parseUBX_NAV_HPPOSLLH(payload);
    } else if (msgClass == UBX_CLASS_NAV && msgID == UBX_NAV_STATUS) {
        parseUBX_NAV_STATUS(payload);
    }

    emit ubxReceived(msg);
}

void ZedF9PReceiver::parseUBX_NAV_PVT(const QByteArray &payload)
{
    if (payload.size() < 84) return;

    // Извлечение данных из NAV-PVT
    qint32 lon = *((qint32*)(payload.data() + 24));
    qint32 lat = *((qint32*)(payload.data() + 28));
    qint32 height = *((qint32*)(payload.data() + 32));
    qint32 hMSL = *((qint32*)(payload.data() + 36));
    quint32 hAcc = *((quint32*)(payload.data() + 40));
    quint32 vAcc = *((quint32*)(payload.data() + 44));

    m_currentData.longitude = lon * 1e-7;
    m_currentData.latitude = lat * 1e-7;
    m_currentData.altitude = hMSL * 0.001;
    m_currentData.accuracyH = hAcc * 0.001;
    m_currentData.accuracyV = vAcc * 0.001;

    quint8 fixType = payload[20];
    quint8 flags = payload[21];

    // Определение типа фикса
    bool carrSoln = (flags >> 6) & 0x03;
    if (carrSoln == 2) {
        m_currentData.rtkFixed = true;
        m_currentData.rtkFloat = false;
        m_currentData.fixType = "RTK Fixed";
    } else if (carrSoln == 1) {
        m_currentData.rtkFloat = true;
        m_currentData.rtkFixed = false;
        m_currentData.fixType = "RTK Float";
    }

    m_currentData.satellites = payload[23];

    emit dataReceived(m_currentData);
}

void ZedF9PReceiver::parseUBX_NAV_HPPOSLLH(const QByteArray &payload)
{
    if (payload.size() < 36) return;

    // Высокоточные координаты
    qint32 lon = *((qint32*)(payload.data() + 8));
    qint32 lat = *((qint32*)(payload.data() + 12));
    qint8 lonHp = payload[16];
    qint8 latHp = payload[17];

    m_currentData.longitude = (lon * 1e-7) + (lonHp * 1e-9);
    m_currentData.latitude = (lat * 1e-7) + (latHp * 1e-9);
}

void ZedF9PReceiver::parseUBX_NAV_STATUS(const QByteArray &payload)
{
    if (payload.size() < 16) return;

    quint8 fixType = payload[4];
    quint8 flags = payload[5];
}

void ZedF9PReceiver::parseRTCM3(const QByteArray &data)
{
    emit rtcmReceived(data);
}

bool ZedF9PReceiver::sendUBXCommand(quint8 msgClass, quint8 msgID, const QByteArray &payload)
{
    if (!m_serialPort->isOpen()) return false;

    QByteArray msg = buildUBXMessage(msgClass, msgID, payload);
    qint64 written = m_serialPort->write(msg);
    m_serialPort->flush();

    return written == msg.size();
}

QByteArray ZedF9PReceiver::buildUBXMessage(quint8 msgClass, quint8 msgID, const QByteArray &payload)
{
    QByteArray msg;
    msg.append((char)UBX_SYNC1);
    msg.append((char)UBX_SYNC2);
    msg.append((char)msgClass);
    msg.append((char)msgID);

    quint16 length = payload.size();
    msg.append((char)(length & 0xFF));
    msg.append((char)((length >> 8) & 0xFF));
    msg.append(payload);

    quint8 ckA = 0, ckB = 0;
    calculateUBXChecksum(msg.mid(2), ckA, ckB);

    msg.append((char)ckA);
    msg.append((char)ckB);

    return msg;
}

void ZedF9PReceiver::calculateUBXChecksum(const QByteArray &data, quint8 &ckA, quint8 &ckB)
{
    ckA = 0;
    ckB = 0;
    for (int i = 0; i < data.size(); i++) {
        ckA += (quint8)data[i];
        ckB += ckA;
    }
}

bool ZedF9PReceiver::configureUBXProtocol()
{
    // Включаем UBX протокол на порту
    QByteArray payload;
    payload.resize(20);
    payload[0] = 1; // UART1
    payload[1] = 0;

    // Настройки порта
    *((quint32*)(payload.data() + 4)) = 0x000008D0; // mode: 8N1
    *((quint32*)(payload.data() + 8)) = 38400; // baudRate

    // Протоколы вход
    *((quint16*)(payload.data() + 12)) = 0x0007; // UBX + NMEA + RTCM3
    // Протоколы выход
    *((quint16*)(payload.data() + 14)) = 0x0003; // UBX + NMEA

    return sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_PRT, payload);
}

bool ZedF9PReceiver::enableNMEAMessages(bool enable)
{
    // Включение/выключение основных NMEA сообщений
    QByteArray payload;
    payload.resize(8);

    const QList<QPair<quint8, quint8>> nmeaMsgs = {
        {0xF0, 0x00}, // GGA
        {0xF0, 0x02}, // GSA
        {0xF0, 0x04}, // RMC
        {0xF0, 0x05}, // VTG
    };

    for (const auto &msg : nmeaMsgs) {
        payload[0] = msg.first;
        payload[1] = msg.second;
        payload[2] = enable ? 1 : 0; // rate UART1

        sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_MSG, payload);
        QThread::msleep(50);
    }

    return true;
}

bool ZedF9PReceiver::enableUBXMessages(bool enable)
{
    QByteArray payload;
    payload.resize(8);

    // NAV-PVT
    payload[0] = UBX_CLASS_NAV;
    payload[1] = UBX_NAV_PVT;
    payload[2] = enable ? 1 : 0;
    sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_MSG, payload);
    QThread::msleep(50);

    // NAV-HPPOSLLH для высокой точности
    payload[1] = UBX_NAV_HPPOSLLH;
    sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_MSG, payload);

    return true;
}

bool ZedF9PReceiver::setNavigationRate(quint16 measRate)
{
    QByteArray payload;
    payload.resize(6);

    *((quint16*)(payload.data() + 0)) = measRate; // Measurement rate (ms)
    *((quint16*)(payload.data() + 2)) = 1; // Navigation rate (cycles)
    *((quint16*)(payload.data() + 4)) = 0; // Time reference: UTC

    return sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_RATE, payload);
}

bool ZedF9PReceiver::enableRTCM3(bool enable)
{
    QByteArray payload;
    payload.resize(8);

    // Основные RTCM3 сообщения
    const QList<quint16> rtcmMsgs = {
        1005, 1077, 1087, 1097, 1127, 1230
    };

    for (quint16 msgNum : rtcmMsgs) {
        payload[0] = 0xF5;
        payload[1] = msgNum & 0xFF;
        payload[2] = enable ? 1 : 0;

        sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_MSG, payload);
        QThread::msleep(50);
    }

    return true;
}

bool ZedF9PReceiver::setRTKMode(bool base)
{
    QByteArray payload;
    payload.resize(40);

    payload[0] = 0; // version
    payload[1] = 0;

    if (base) {
        // Режим базовой станции (Survey-in)
        *((quint8*)(payload.data() + 2)) = 1; // timeMode: Survey-in

        // Параметры Survey-in
        *((quint32*)(payload.data() + 24)) = 300; // svinMinDur (сек)
        *((quint32*)(payload.data() + 28)) = 50000; // svinAccLimit (0.1mm)
    } else {
        // Режим ровера
        *((quint8*)(payload.data() + 2)) = 0; // timeMode: Disabled
    }

    return sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_TMODE3, payload);
}

bool ZedF9PReceiver::sendRTCMCorrections(const QByteArray &rtcmData)
{
    if (!m_serialPort->isOpen()) return false;

    qint64 written = m_serialPort->write(rtcmData);
    m_serialPort->flush();

    return written == rtcmData.size();
}

bool ZedF9PReceiver::saveConfiguration()
{
    QByteArray payload;
    payload.resize(13);

    *((quint32*)(payload.data() + 0)) = 0xFFFFFFFF; // Clear mask
    *((quint32*)(payload.data() + 4)) = 0xFFFFFFFF; // Save mask
    *((quint32*)(payload.data() + 8)) = 0x00000000; // Load mask
    payload[12] = 0xFF; // Save to all devices

    return sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_CFG, payload);
}

bool ZedF9PReceiver::resetToDefaults()
{
    QByteArray payload;
    payload.resize(13);

    *((quint32*)(payload.data() + 0)) = 0xFFFFFFFF; // Clear all
    *((quint32*)(payload.data() + 4)) = 0x00000000;
    *((quint32*)(payload.data() + 8)) = 0xFFFFFFFF; // Load defaults
    payload[12] = 0xFF;

    return sendUBXCommand(UBX_CLASS_CFG, UBX_CFG_CFG, payload);
}

bool ZedF9PReceiver::validateNMEAChecksum(const QString &sentence)
{
    int asteriskPos = sentence.indexOf('*');
    if (asteriskPos == -1) return false;

    QString data = sentence.mid(1, asteriskPos - 1);
    QString checksumStr = sentence.mid(asteriskPos + 1, 2);

    quint8 checksum = 0;
    for (QChar c : data) {
        checksum ^= c.toLatin1();
    }

    bool ok;
    quint8 receivedChecksum = checksumStr.toUInt(&ok, 16);

    return ok && (checksum == receivedChecksum);
}

double ZedF9PReceiver::nmeaToDecimal(const QString &coord, const QString &direction)
{
    if (coord.isEmpty()) return 0.0;

    int dotPos = coord.indexOf('.');
    if (dotPos < 3) return 0.0;

    double degrees = coord.left(dotPos - 2).toDouble();
    double minutes = coord.mid(dotPos - 2).toDouble();

    double decimal = degrees + (minutes / 60.0);

    if (direction == "S" || direction == "W") {
        decimal = -decimal;
    }

    return decimal;
}

void ZedF9PReceiver::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }

    if (!m_serialPort->isOpen()) {
        return;
    }

    QString errorString = m_serialPort->errorString();

    if (error == QSerialPort::ResourceError ||
            error == QSerialPort::PermissionError ||
            error == QSerialPort::DeviceNotFoundError) {
        emit errorOccurred("Ошибка порта: " + errorString);

        disconnectFromReceiver();
    }
}

