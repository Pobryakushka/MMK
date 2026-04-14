#include "customprotocol.h"
#include <QDebug>
#include <cstring>

CustomProtocol::CustomProtocol(QObject *parent)
    : QObject(parent)
{
}

CustomProtocol::~CustomProtocol()
{
}

QByteArray CustomProtocol::buildRequestPacket()
{
    QByteArray packet;
    
    // Начало пакета
    packet.append((char)PACKET_START);
    packet.append((char)FLAG);
    packet.append((char)PACKET_ID);
    
    // Для запроса данных - пустая информационная часть
    // Контрольная сумма = 0
    QByteArray checksum;
    checksum.append((char)0x00);
    checksum.append((char)0x00);
    
    // Экранируем контрольную сумму если нужно
    QByteArray escapedChecksum = escapeData(checksum);
    packet.append(escapedChecksum);
    
    // Конец пакета
    packet.append((char)FLAG);
    packet.append((char)END_OF_TEXT);
    
    return packet;
}

bool CustomProtocol::parsePacket(const QByteArray &data, CustomNavigationData &navData)
{
    if (data.size() < 5) {
        emit parseError("Пакет слишком короткий");
        return false;
    }
    
    // Проверяем начало пакета
    if ((quint8)data[0] != PACKET_START || (quint8)data[1] != FLAG) {
        emit parseError("Неверное начало пакета");
        return false;
    }
    
    // Проверяем идентификатор
    if ((quint8)data[2] != PACKET_ID) {
        emit parseError("Неверный идентификатор пакета");
        return false;
    }
    
    // Ищем конец пакета
    int endPos = -1;
    for (int i = 3; i < data.size() - 1; i++) {
        if ((quint8)data[i] == FLAG && (quint8)data[i + 1] == END_OF_TEXT) {
            endPos = i;
            break;
        }
    }
    
    if (endPos == -1) {
        emit parseError("Не найден конец пакета");
        return false;
    }
    
    // Извлекаем информационную часть с контрольной суммой
    QByteArray escapedPayload = data.mid(3, endPos - 3);
    
    // Деэкранируем данные
    QByteArray payload = unescapeData(escapedPayload);
    
    if (payload.size() < 2) {
        emit parseError("Слишком короткая информационная часть");
        return false;
    }
    
    // Извлекаем контрольную сумму (последние 2 байта)
    quint16 receivedChecksum = (quint8)payload[payload.size() - 2] | 
                               ((quint8)payload[payload.size() - 1] << 8);
    
    // Данные без контрольной суммы
    QByteArray dataOnly = payload.left(payload.size() - 2);
    
    // Вычисляем контрольную сумму
    quint16 calculatedChecksum = calculateChecksum(dataOnly);
    
    if (receivedChecksum != calculatedChecksum) {
        emit parseError(QString("Ошибка контрольной суммы: получено %1, вычислено %2")
                        .arg(receivedChecksum).arg(calculatedChecksum));
        return false;
    }
    
    // Разбираем данные
    int offset = 0;
    
    if (!decodeDouble(dataOnly, offset, navData.time)) return false;
    if (!decodeWord(dataOnly, offset, navData.coordinateSystem)) return false;
    if (!decodeWord(dataOnly, offset, navData.projection)) return false;
    if (!decodeDouble(dataOnly, offset, navData.latitude)) return false;
    if (!decodeDouble(dataOnly, offset, navData.longitude)) return false;
    if (!decodeDouble(dataOnly, offset, navData.altitude)) return false;
    if (!decodeFloat(dataOnly, offset, navData.umshv)) return false;
    if (!decodeFloat(dataOnly, offset, navData.v2d)) return false;
    if (!decodeFloat(dataOnly, offset, navData.az)) return false;
    if (!decodeFloat(dataOnly, offset, navData.vh)) return false;
    if (!decodeFloat(dataOnly, offset, navData.f)) return false;
    if (!decodeFloat(dataOnly, offset, navData.gps_glo)) return false;
    if (!decodeFloat(dataOnly, offset, navData.dt)) return false;
    if (!decodeFloat(dataOnly, offset, navData.pdop)) return false;
    if (!decodeFloat(dataOnly, offset, navData.vdop)) return false;
    if (!decodeFloat(dataOnly, offset, navData.hdop)) return false;
    if (!decodeFloat(dataOnly, offset, navData.tdop)) return false;
    if (!decodeUInt32(dataOnly, offset, navData.sat_glo)) return false;
    if (!decodeUInt32(dataOnly, offset, navData.sat_gps)) return false;
    if (!decodeWord(dataOnly, offset, navData.dispersion)) return false;
    
    navData.timestamp = QDateTime::currentDateTime();
    
    return true;
}

QByteArray CustomProtocol::buildDataPacket(const CustomNavigationData &navData)
{
    QByteArray packet;
    QByteArray dataPayload;

    // Собираем информационную часть
    dataPayload.append(encodeDouble(navData.time));
    dataPayload.append(encodeWord(navData.coordinateSystem));
    dataPayload.append(encodeWord(navData.projection));
    dataPayload.append(encodeDouble(navData.latitude));
    dataPayload.append(encodeDouble(navData.longitude));
    dataPayload.append(encodeDouble(navData.altitude));
    dataPayload.append(encodeFloat(navData.umshv));
    dataPayload.append(encodeFloat(navData.v2d));
    dataPayload.append(encodeFloat(navData.az));
    dataPayload.append(encodeFloat(navData.vh));
    dataPayload.append(encodeFloat(navData.f));
    dataPayload.append(encodeFloat(navData.gps_glo));
    dataPayload.append(encodeFloat(navData.dt));
    dataPayload.append(encodeFloat(navData.pdop));
    dataPayload.append(encodeFloat(navData.vdop));
    dataPayload.append(encodeFloat(navData.hdop));
    dataPayload.append(encodeFloat(navData.tdop));
    dataPayload.append(encodeUInt32(navData.sat_glo));
    dataPayload.append(encodeUInt32(navData.sat_gps));
    dataPayload.append(encodeWord(navData.dispersion));

    // Вычисляем контрольную сумму ДО экранирования
    quint16 checksum = calculateChecksum(dataPayload);

    // Добавляем контрольную сумму к данным
    QByteArray checksumBytes;
    checksumBytes.append((char)(checksum & 0xFF));         // Младший байт
    checksumBytes.append((char)((checksum >> 8) & 0xFF));  // Старший байт

    // Экранируем данные вместе с контрольной суммой
    QByteArray escapedData = escapeData(dataPayload + checksumBytes);

    // Собираем пакет
    packet.append((char)PACKET_START);  // 0x20
    packet.append((char)FLAG);          // 0x10
    packet.append((char)PACKET_ID);     // 0x2A
    packet.append(escapedData);         // Экранированные данные + КС
    packet.append((char)FLAG);          // 0x10
    packet.append((char)END_OF_TEXT);   // 0x03

    return packet;
}

void CustomProtocol::addData(const QByteArray &data)
{
    m_buffer.append(data);
    processBuffer();
}

void CustomProtocol::processBuffer()
{
    while (true) {
        // Ищем начало пакета
        int startPos = -1;
        for (int i = 0; i < m_buffer.size() - 1; i++) {
            if ((quint8)m_buffer[i] == PACKET_START && (quint8)m_buffer[i + 1] == FLAG) {
                startPos = i;
                break;
            }
        }
        
        if (startPos == -1) {
            m_buffer.clear();
            break;
        }
        
        // Удаляем все до начала пакета
        if (startPos > 0) {
            m_buffer.remove(0, startPos);
        }
        
        // Ищем конец пакета
        int endPos = -1;
        for (int i = 3; i < m_buffer.size() - 1; i++) {
            if ((quint8)m_buffer[i] == FLAG && (quint8)m_buffer[i + 1] == END_OF_TEXT) {
                endPos = i + 1;
                break;
            }
        }
        
        if (endPos == -1) {
            // Полный пакет еще не получен
            break;
        }
        
        // Извлекаем пакет
        QByteArray packet = m_buffer.left(endPos + 1);
        m_buffer.remove(0, endPos + 1);
        
        // Разбираем пакет
        CustomNavigationData navData;
        if (parsePacket(packet, navData)) {
            emit navigationDataReceived(navData);
        }
    }
}

QByteArray CustomProtocol::escapeData(const QByteArray &data)
{
    QByteArray escaped;
    for (int i = 0; i < data.size(); i++) {
        escaped.append(data[i]);
        if ((quint8)data[i] == FLAG) {
            escaped.append((char)FLAG);  // Повторяем байт 0x10
        }
    }
    return escaped;
}

QByteArray CustomProtocol::unescapeData(const QByteArray &data)
{
    QByteArray unescaped;
    for (int i = 0; i < data.size(); i++) {
        unescaped.append(data[i]);
        if ((quint8)data[i] == FLAG && i + 1 < data.size() && (quint8)data[i + 1] == FLAG) {
            i++;  // Пропускаем следующий байт 0x10
        }
    }
    return unescaped;
}

quint16 CustomProtocol::calculateChecksum(const QByteArray &data)
{
    quint16 checksum = 0;
    
    // Суммируем по словам (16 бит)
    for (int i = 0; i < data.size(); i += 2) {
        quint16 word;
        if (i + 1 < data.size()) {
            word = (quint8)data[i] | ((quint8)data[i + 1] << 8);
        } else {
            word = (quint8)data[i];
        }
        checksum += word;
    }
    
    return checksum;
}

QByteArray CustomProtocol::encodeWord(quint16 value)
{
    QByteArray result;
    result.append((char)(value & 0xFF));         // Младший байт
    result.append((char)((value >> 8) & 0xFF));  // Старший байт
    return result;
}

QByteArray CustomProtocol::encodeDouble(double value)
{
    QByteArray result;
    quint64 bits;
    std::memcpy(&bits, &value, sizeof(double));
    
    for (int i = 0; i < 8; i++) {
        result.append((char)((bits >> (i * 8)) & 0xFF));
    }
    return result;
}

QByteArray CustomProtocol::encodeFloat(float value)
{
    QByteArray result;
    quint32 bits;
    std::memcpy(&bits, &value, sizeof(float));
    
    for (int i = 0; i < 4; i++) {
        result.append((char)((bits >> (i * 8)) & 0xFF));
    }
    return result;
}

QByteArray CustomProtocol::encodeUInt32(quint32 value)
{
    QByteArray result;
    for (int i = 0; i < 4; i++) {
        result.append((char)((value >> (i * 8)) & 0xFF));
    }
    return result;
}

bool CustomProtocol::decodeWord(const QByteArray &data, int &offset, quint16 &value)
{
    if (offset + 2 > data.size()) {
        emit parseError("Недостаточно данных для чтения Word");
        return false;
    }
    
    value = (quint8)data[offset] | ((quint8)data[offset + 1] << 8);
    offset += 2;
    return true;
}

bool CustomProtocol::decodeDouble(const QByteArray &data, int &offset, double &value)
{
    if (offset + 8 > data.size()) {
        emit parseError("Недостаточно данных для чтения Double");
        return false;
    }
    
    quint64 bits = 0;
    for (int i = 0; i < 8; i++) {
        bits |= ((quint64)(quint8)data[offset + i]) << (i * 8);
    }
    
    std::memcpy(&value, &bits, sizeof(double));
    offset += 8;
    return true;
}

bool CustomProtocol::decodeFloat(const QByteArray &data, int &offset, float &value)
{
    if (offset + 4 > data.size()) {
        emit parseError("Недостаточно данных для чтения Float");
        return false;
    }
    
    quint32 bits = 0;
    for (int i = 0; i < 4; i++) {
        bits |= ((quint32)(quint8)data[offset + i]) << (i * 8);
    }
    
    std::memcpy(&value, &bits, sizeof(float));
    offset += 4;
    return true;
}

bool CustomProtocol::decodeUInt32(const QByteArray &data, int &offset, quint32 &value)
{
    if (offset + 4 > data.size()) {
        emit parseError("Недостаточно данных для чтения UInt32");
        return false;
    }
    
    value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((quint32)(quint8)data[offset + i]) << (i * 8);
    }
    
    offset += 4;
    return true;
}
