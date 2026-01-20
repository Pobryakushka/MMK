#include "amsprotocol.h"
#include <QDebug>
#include <QtEndian>

AMSProtocol::AMSProtocol(QObject *parent)
    : QObject(parent)
{
}

// ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====

quint8 AMSProtocol::calculateChecksum(const QByteArray &data)
{
    // Контрольная сумма = сумма по модулю 2 всех байтов кроме номера команды
    quint8 checksum = 0;
    for (int i = 1; i < data.size(); i++) {
        checksum ^= static_cast<quint8>(data[i]);
    }
    return checksum;
}

QByteArray AMSProtocol::finalizePacket(const QByteArray &data)
{
    QByteArray packet = data;
    packet.append(calculateChecksum(data));
    packet.append(static_cast<char>(0xFF)); // Стоповый байт
    return packet;
}

QByteArray AMSProtocol::floatToBytes(float value)
{
    QByteArray bytes;
    bytes.resize(4);
    qToLittleEndian(value, reinterpret_cast<uchar*>(bytes.data()));
    return bytes;
}

QByteArray AMSProtocol::intToBytes(qint32 value)
{
    QByteArray bytes;
    bytes.resize(4);
    qToLittleEndian(value, reinterpret_cast<uchar*>(bytes.data()));
    return bytes;
}

float AMSProtocol::bytesToFloat(const QByteArray &data, int offset)
{
    if (offset + 4 > data.size()) return 0.0f;
    return qFromLittleEndian<float>(reinterpret_cast<const uchar*>(data.data() + offset));
}

qint32 AMSProtocol::bytesToInt(const QByteArray &data, int offset)
{
    if (offset + 4 > data.size()) return 0;
    return qFromLittleEndian<qint32>(reinterpret_cast<const uchar*>(data.data() + offset));
}

bool AMSProtocol::isPacketValid(const QByteArray &data)
{
    if (data.size() < 3) return false;
    if (static_cast<quint8>(data.back()) != 0xFF) return false;
    
    quint8 receivedChecksum = static_cast<quint8>(data[data.size() - 2]);
    QByteArray dataWithoutChecksumAndStop = data.left(data.size() - 2);
    quint8 calculatedChecksum = calculateChecksum(dataWithoutChecksumAndStop);
    
    return receivedChecksum == calculatedChecksum;
}

AMSCommand AMSProtocol::getPacketCommand(const QByteArray &data)
{
    if (data.isEmpty()) return static_cast<AMSCommand>(0x00);
    return static_cast<AMSCommand>(static_cast<quint8>(data[0]));
}

// ===== СОЗДАНИЕ ПАКЕТОВ =====

QByteArray AMSProtocol::createLineTestPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_LINE_TEST));
    packet.append(QByteArray::fromHex("FFFFFFFF"));
    packet.append(QByteArray::fromHex("EEEEEEEE"));
    packet.append(QByteArray::fromHex("DDDDDDDD"));
    packet.append(QByteArray::fromHex("CCCCCC00"));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createModeTransferPacket(WorkMode mode, Litera litera)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_MODE_TRANSFER));
    packet.append(static_cast<char>(mode));
    packet.append(static_cast<char>(litera));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createCoordsTransferPacket(const StationCoordinates &coords)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_COORDS_TRANSFER));
    packet.append(intToBytes(coords.latitude));
    packet.append(intToBytes(coords.longitude));
    packet.append(floatToBytes(coords.altitude));
    packet.append(floatToBytes(coords.azimuth));
    packet.append(floatToBytes(coords.pitch));
    packet.append(floatToBytes(coords.roll));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createStartMeasurementPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_START_MEASUREMENT));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createDataExchangePacket(bool continueProcess)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_DATA_EXCHANGE));
    packet.append(continueProcess ? static_cast<char>(0x00) : static_cast<char>(0x01));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createSourceDataPacket(int day, int hour, int tenMinutes,
                                               float stationAltitude,
                                               const QVector<float> &avgWindDir,
                                               const QVector<float> &avgWindSpeed,
                                               float reachedHeight,
                                               float surfaceWindDir, float surfaceWindSpeed,
                                               const QDateTime &currentDateTime)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_SOURCE_DATA));
    packet.append(intToBytes(day));
    packet.append(intToBytes(hour));
    packet.append(intToBytes(tenMinutes));
    packet.append(floatToBytes(stationAltitude));
    
    // 23 значения направления ветра
    for (int i = 0; i < 23; i++) {
        packet.append(floatToBytes(i < avgWindDir.size() ? avgWindDir[i] : 0.0f));
    }
    
    // 23 значения скорости ветра
    for (int i = 0; i < 23; i++) {
        packet.append(floatToBytes(i < avgWindSpeed.size() ? avgWindSpeed[i] : 0.0f));
    }
    
    packet.append(floatToBytes(reachedHeight));
    packet.append(floatToBytes(surfaceWindDir));
    packet.append(floatToBytes(surfaceWindSpeed));
    
    // Дата/время в формате ММДДччммГГГГ
    QString dateTimeStr = currentDateTime.toString("MMddhhmmyyyy");
    packet.append(dateTimeStr.toLatin1());
    
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createFuncControlPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_FUNC_CONTROL));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createAvgWindRequestPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_AVG_WIND_REQUEST));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createActualWindRequestPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_ACTUAL_WIND_REQUEST));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createMeasuredWindRequestPacket()
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_MEASURED_WIND_REQUEST));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createAntennaControlPacket(quint8 command)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_ANTENNA_CONTROL));
    packet.append(static_cast<char>(command));
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createSetDateTimePacket(const QDateTime &dateTime)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_SET_DATETIME));
    QString dateTimeStr = dateTime.toString("MMddhhmmyyyy");
    packet.append(dateTimeStr.toLatin1());
    return finalizePacket(packet);
}

QByteArray AMSProtocol::createRotateAntennaPacket(quint8 command, float angle)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_ROTATE_ANTENNA));
    packet.append(static_cast<char>(command));
    packet.append(floatToBytes(angle));
    return finalizePacket(packet);
}

// ===== ПАРСИНГ ОТВЕТОВ =====

bool AMSProtocol::parseLineTestResponse(const QByteArray &data)
{
    if (!isPacketValid(data)) return false;
    if (getPacketCommand(data) != CMD_LINE_TEST) return false;
    
    // Проверяем, что ответ содержит те же тестовые данные
    QByteArray expectedTest;
    expectedTest.append(QByteArray::fromHex("FFFFFFFF"));
    expectedTest.append(QByteArray::fromHex("EEEEEEEE"));
    expectedTest.append(QByteArray::fromHex("DDDDDDDD"));
    expectedTest.append(QByteArray::fromHex("CCCCCC00"));
    
    if (data.size() < 1 + expectedTest.size() + 2) return false;
    
    QByteArray receivedTest = data.mid(1, expectedTest.size());
    return receivedTest == expectedTest;
}

bool AMSProtocol::parseModeTransferResponse(const QByteArray &data)
{
    if (!isPacketValid(data)) return false;
    return getPacketCommand(data) == CMD_MODE_TRANSFER && data.size() == 3;
}

bool AMSProtocol::parseCoordsTransferResponse(const QByteArray &data)
{
    if (!isPacketValid(data)) return false;
    return getPacketCommand(data) == CMD_COORDS_TRANSFER && data.size() == 3;
}

WorkMode AMSProtocol::parseStartMeasurementResponse(const QByteArray &data, bool &ok)
{
    ok = false;
    if (!isPacketValid(data)) return MODE_WORKING;
    if (getPacketCommand(data) != CMD_START_MEASUREMENT) return MODE_WORKING;
    if (data.size() < 4) return MODE_WORKING;
    
    ok = true;
    quint8 mode = static_cast<quint8>(data[1]);
    return static_cast<WorkMode>(mode);
}

MeasurementProgress AMSProtocol::parseDataExchangeResponse(const QByteArray &data, bool &ok)
{
    MeasurementProgress progress;
    ok = false;
    
    if (!isPacketValid(data)) return progress;
    if (getPacketCommand(data) != CMD_DATA_EXCHANGE) return progress;
    if (data.size() < 11) return progress;
    
    progress.percentComplete = bytesToInt(data, 1);
    progress.currentAngle = bytesToFloat(data, 5);
    ok = true;
    
    return progress;
}

bool AMSProtocol::parseSourceDataResponse(const QByteArray &data)
{
    if (!isPacketValid(data)) return false;
    return getPacketCommand(data) == CMD_SOURCE_DATA && data.size() == 3;
}

bool AMSProtocol::parseFuncControlResponse(const QByteArray &data, quint32 &bitMask, quint32 &powerOnCount)
{
    if (!isPacketValid(data)) return false;
    if (getPacketCommand(data) != CMD_FUNC_CONTROL) return false;
    if (data.size() < 11) return false;
    
    bitMask = static_cast<quint32>(bytesToInt(data, 1));
    powerOnCount = static_cast<quint32>(bytesToInt(data, 5));
    
    return true;
}

QVector<WindProfileData> AMSProtocol::parseAvgWindResponse(const QByteArray &data, bool &ok)
{
    QVector<WindProfileData> profile;
    ok = false;

    if (!isPacketValid(data)) return profile;
    if (getPacketCommand(data) != CMD_AVG_WIND_REQUEST) return profile;
    if (data.size() < 131) return profile;

    // 16 уровней высоты - ДОБАВЛЯЕМ РАСЧЕТ ВЫСОТЫ
    for (int i = 0; i < 16; i++) {
        WindProfileData point;
        point.height = 100.0f + i * 200.0f; // Высоты: 100, 300, 500, ..., 3100 м
        point.windDirection = static_cast<int>(bytesToFloat(data, 1 + i * 4));
        point.windSpeed = bytesToFloat(data, 1 + 64 + i * 4);
        point.isValid = true;
        profile.append(point);
    }

    ok = true;
    return profile;
}

QVector<WindProfileData> AMSProtocol::parseActualWindResponse(const QByteArray &data, bool &ok)
{
    QVector<WindProfileData> profile;
    ok = false;

    if (!isPacketValid(data)) return profile;
    if (getPacketCommand(data) != CMD_ACTUAL_WIND_REQUEST) return profile;
    if (data.size() < 243) return profile;

    // 30 уровней высоты - ДОБАВЛЯЕМ РАСЧЕТ ВЫСОТЫ
    for (int i = 0; i < 30; i++) {
        WindProfileData point;
        point.height = 100.0f + i * 100.0f; // Высоты: 100, 200, 300, ..., 3000 м
        point.windDirection = static_cast<int>(bytesToFloat(data, 1 + i * 4));
        point.windSpeed = bytesToFloat(data, 1 + 120 + i * 4);
        point.isValid = true;
        profile.append(point);
    }

    ok = true;
    return profile;
}

QVector<MeasuredWindData> AMSProtocol::parseMeasuredWindResponse(const QByteArray &data, bool &ok)
{
    QVector<MeasuredWindData> profile;
    ok = false;
    
    if (!isPacketValid(data)) return profile;
    if (getPacketCommand(data) != CMD_MEASURED_WIND_REQUEST) return profile;
    if (data.size() < 1603) return profile; // 1 + 1600 + 1 + 1
    
    // 100 измерений по 16 байт каждое
    for (int i = 0; i < 100; i++) {
        int offset = 1 + i * 16;
        MeasuredWindData point;
        point.windSpeed = bytesToFloat(data, offset);
        point.windDirection = static_cast<int>(bytesToFloat(data, offset + 4));
        point.height = bytesToFloat(data, offset + 8);
        point.reliability = bytesToInt(data, offset + 12);
        profile.append(point);
    }
    
    ok = true;
    return profile;
}

quint8 AMSProtocol::parseAntennaControlResponse(const QByteArray &data, bool &ok)
{
    ok = false;
    if (!isPacketValid(data)) return 0;
    if (getPacketCommand(data) != CMD_ANTENNA_CONTROL) return 0;
    if (data.size() < 4) return 0;
    
    ok = true;
    return static_cast<quint8>(data[1]);
}

bool AMSProtocol::parseSetDateTimeResponse(const QByteArray &data)
{
    if (!isPacketValid(data)) return false;
    return getPacketCommand(data) == CMD_SET_DATETIME && data.size() == 3;
}

bool AMSProtocol::parseRotateAntennaResponse(const QByteArray &data, quint8 &status, float &currentAngle)
{
    if (!isPacketValid(data)) return false;
    if (getPacketCommand(data) != CMD_ROTATE_ANTENNA) return false;
    if (data.size() < 8) return false;
    
    status = static_cast<quint8>(data[1]);
    currentAngle = bytesToFloat(data, 2);
    
    return true;
}
