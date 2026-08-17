#include "binshandler.h"
#include <QDebug>
#include <QtEndian>

BINSHandler::BINSHandler(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_connectionTimer(new QTimer(this))
{
    connect(m_serialPort, &QSerialPort::readyRead,
            this, &BINSHandler::onSerialDataReceived);
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &BINSHandler::onSerialError);

    m_connectionTimer->setSingleShot(true);
    connect(m_connectionTimer, &QTimer::timeout,
            this, &BINSHandler::onConnectionTimeout);
}

BINSHandler::~BINSHandler()
{
    disconnectFromBINS();
}

bool BINSHandler::connectToBINS(const QString &portName,
                                quint32 baudRate,
                                QSerialPort::DataBits dataBits,
                                QSerialPort::Parity parity,
                                QSerialPort::StopBits stopBits)
{
    if (m_serialPort->isOpen()) {
        qWarning() << "БИНС: Порт уже открыт";
        return false;
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadOnly)) {
        QString error = QString("Не удалось открыть порт: %1")
                        .arg(m_serialPort->errorString());
        emit errorOccurred(error);
        qCritical() << "БИНС: " << error;
        return false;
    }

    m_receiveBuffer.clear();
    m_currentData = BINSData();

    // Запускаем таймер ожидания данных
    m_connectionTimer->start(CONNECTION_TIMEOUT);
    qInfo() << "БИНС: Подключено к" << portName <<  "(" << baudRate << "бод, RS-232)";
    qInfo() << "БИНС: Ожидание данных...";
    return true;
}

void BINSHandler::disconnectFromBINS()
{
    m_connectionTimer->stop();

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        qInfo() << "БИНС: Отключено";
        emit disconnected();
    }

    m_receiveBuffer.clear();
    m_currentData = BINSData();
}

bool BINSHandler::isConnected() const
{
    return m_serialPort->isOpen();
}

void BINSHandler::onSerialDataReceived()
{
    QByteArray newData = m_serialPort->readAll();
    m_receiveBuffer.append(newData);

    // ОТЛАДКА: показываем что получили
    qDebug() << "БИНС: Получено байт:" << newData.size() << "HEX:" << newData.toHex(' ').toUpper();

    // Первые данные - подтверждаем подключение
    if (m_connectionTimer->isActive()) {
        m_connectionTimer->stop();
        emit connected();
        emit statusMessage("Прием данных от БИНС");
        qInfo() << "БИНС: Данные получены, подключение установлено";
    }

    processReceivedData();
}

void BINSHandler::processReceivedData()
{
    while (m_receiveBuffer.size() >= MIN_PACKET_SIZE) {
        // Ищем заголовок пакета
        int headerPos = m_receiveBuffer.indexOf(PACKET_HEADER);

        if (headerPos == -1) {
            // Заголовок не найден, очищаем буфер
            qWarning() << "БИНС: Заголовок 0xAA не найден, очищаем буфер";
            m_receiveBuffer.clear();
            return;
        }

        if (headerPos > 0) {
            // Удаляем мусор до заголовка
            qDebug() << "БИНС: Пропускаем" << headerPos << "байт до заголовка";
            m_receiveBuffer.remove(0, headerPos);
        }

        // Проверяем, достаточно ли данных для пакета
        if (m_receiveBuffer.size() < MIN_PACKET_SIZE) {
            qDebug() << "БИНС: Недостаточно данных для пакета. Есть:" << m_receiveBuffer.size()
                     << "Нужно:" << MIN_PACKET_SIZE;
            return; // Ждем больше данных
        }

        // Проверяем тип пакета
        quint8 packetType = static_cast<quint8>(m_receiveBuffer[1]);

        if (packetType != PACKET_TYPE_ORIENT) {
            qWarning() << "БИНС: Неизвестный тип пакета:" << Qt::hex << packetType;
            m_receiveBuffer.remove(0, 1);
            continue;
        }

        // Извлекаем пакет
        QByteArray packet = m_receiveBuffer.left(MIN_PACKET_SIZE);

        qDebug() << "БИНС: Полный пакет (" << packet.size() << "байт):" << packet.toHex(' ').toUpper();

        // Проверяем CRC
        QByteArray dataForCrc = packet.left(DATA_SIZE);
        qDebug() << "БИНС: Данные для CRC (" << dataForCrc.size() << "байт):" << dataForCrc.toHex(' ').toUpper();

        quint16 receivedCrc = extractUInt16(packet, DATA_SIZE);
        quint16 calculatedCrc = calculateCRC16(dataForCrc);

        qDebug() << QString("БИНС: CRC получен: 0x%1, вычислен: 0x%2")
                    .arg(receivedCrc, 4, 16, QChar('0'))
                    .arg(calculatedCrc, 4, 16, QChar('0'));

        if (receivedCrc != calculatedCrc) {
            qWarning() << QString("БИНС: ❌ ОШИБКА CRC! Получено: 0x%1, Ожидалось: 0x%2")
                          .arg(receivedCrc, 4, 16, QChar('0'))
                          .arg(calculatedCrc, 4, 16, QChar('0'));
            m_receiveBuffer.remove(0, 1);
            continue;
        }

        qDebug() << "БИНС: ✓ CRC совпадает!";

        // Парсим пакет
        if (parseOrientationPacket(packet)) {
            emit dataReceived(m_currentData);
        }

        // Удаляем обработанный пакет из буфера
        m_receiveBuffer.remove(0, MIN_PACKET_SIZE);
    }
}

bool BINSHandler::parseOrientationPacket(const QByteArray &packet)
{
    if (packet.size() != MIN_PACKET_SIZE) {
        return false;
    }

    int offset = 2; // Пропускаем заголовок и тип

    // Счетчик пакетов
    m_currentData.packetCounter = extractUInt16(packet, offset);
    offset += 2;

    // Углы ориентации
    m_currentData.heading = static_cast<double>(extractFloat(packet, offset));
    offset += 4;

    m_currentData.roll = static_cast<double>(extractFloat(packet, offset));
    offset += 4;

    m_currentData.pitch = static_cast<double>(extractFloat(packet, offset));
    offset += 4;

    m_currentData.valid = true;

    qInfo() << QString("БИНС [%1]: Курс=%2° Крен=%3° Тангаж=%4°")
                .arg(m_currentData.packetCounter)
                .arg(m_currentData.heading, 0, 'f', 2)
                .arg(m_currentData.roll, 0, 'f', 2)
                .arg(m_currentData.pitch, 0, 'f', 2);

    return true;
}

void BINSHandler::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }

    QString errorStr = QString("Ошибка порта: %1").arg(m_serialPort->errorString());
    emit errorOccurred(errorStr);
    qCritical() << "БИНС:" << errorStr;
}

void BINSHandler::onConnectionTimeout()
{
    if (m_serialPort->isOpen()) {
        QString error = "Таймаут ожидания данных от БИНС";
        emit errorOccurred(error);
        qWarning() << "БИНС:" << error;
        disconnectFromBINS();
    }
}

// ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====

quint16 BINSHandler::calculateCRC16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    qDebug() << "CRC: Начало вычисления, размер данных:" << data.size();

    for (int i = 0; i < data.size(); i++) {
        crc ^= (static_cast<quint16>(static_cast<quint8>(data[i])) << 8);

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }

    qDebug() << QString("CRC: Результат = 0x%1").arg(crc, 4, 16, QChar('0'));
    return crc;
}

float BINSHandler::extractFloat(const QByteArray &data, int offset)
{
    if (offset + 4 > data.size()) return 0.0f;

    union {
        float f;
        quint32 i;
    } converter;

    converter.i = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData() + offset));

    return converter.f;
}

double BINSHandler::extractDouble(const QByteArray &data, int offset)
{
    if (offset + 8 > data.size()) return 0.0;

    union {
        double d;
        quint64 i;
    } converter;

    converter.i = qFromLittleEndian<quint64>(reinterpret_cast<const uchar*>(data.constData() + offset));

    return converter.d;
}

quint16 BINSHandler::extractUInt16(const QByteArray &data, int offset)
{
    if (offset + 2 > data.size()) return 0;

    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(data.constData() + offset));
}

quint32 BINSHandler::extractUInt32(const QByteArray &data, int offset)
{
    if (offset + 4 > data.size()) return 0;

    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(data.constData() + offset));
}
