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
    for (int i = 0; i < data.size(); i++) {
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
    qDebug() << "Размер пакета: " << data.size();
    qDebug() << "Результат сравнения: " <<static_cast<quint8>(data.back());

    if (data.size() < 3) return false;
    if (static_cast<quint8>(data.back()) != 0xFF) return false;

    quint8 receivedChecksum = static_cast<quint8>(data[data.size() - 2]);
    QByteArray dataWithoutChecksumAndStop = data.left(data.size() - 2);
    quint8 calculatedChecksum = calculateChecksum(dataWithoutChecksumAndStop);

    qDebug() << "Полученная КС" << receivedChecksum;
        qDebug() << "Вычисленная КС" << calculatedChecksum;

    return receivedChecksum == calculatedChecksum;
}

AMSCommand AMSProtocol::getPacketCommand(const QByteArray &data)
{
    if (data.size() < 3) return PARSE_ERR_TOO_SHORT;
    if (static_cast<quint8>(data.back()) != 0xFF) return PARSE_ERR_BAD_STOP;

    quint8 receivedCs = static_cast<quint8>(data[data.size() - 2]);
    quint8 calcCs = calculateChecksum(data.left(data.size() - 2));
    if (receivedCs != calcCs) return PARSE_ERR_BAD_CHECKSUM;

    if (getPacketCommand(data) != expectedCmd) return PARSE_ERR_WRONG_COMMAND;
    if (data.size() < minSize) return PARSE_ERR_TOO_SHORT;

    return PARSE_OK;
}

QString AMSProtocol::parseErrorString(ParseError err)
{
    switch (err) {
    case PARSE_OK:               return "OK";
    case PARSE_ERR_TOO_SHORT:    return "Пакет слишком короткий";
    case PARSE_ERR_BAD_STOP:     return "Отсутствует стоповый байт 0xFF";
    case PARSE_ERR_BAD_CHECKSUM: return "Ошибка контрольной суммы";
    case PARSE_ERR_WRONG_COMMAND:return "Неожиданная команда в ответе";
    case PARSE_ERR_BAD_DATA:     return "Некорректные данные в пакете";
    }
    return "Неизвестная ошибка";
}

QString AMSProtocol::antennaStatusString(quint8 status)
{
    switch (status) {
    case ANTENNA_IN_PROGRESS: return "Процесс открытия/закрытия антенны";
    case ANTENNA_SUCCESS:     return "Антенна: завершено успешно";
    case ANTENNA_FAULT:       return "Антенна: аварийная остановка";
    }
    return QString("Антенна: неизвестный статус 0x%1").arg(status, 2, 16, QChar('0'));
}

QString AMSProtocol::rotateStatusString(quint8 status)
{
    switch (status) {
    case ROTATE_IDLE_OK: return "Поворот: ожидание / завершено успешно";
    case ROTATE_RUNNING: return "Поворот: вращение";
    case ROTATE_FAULT:   return "Поворот: аварийная остановка";
    }
    return QString("Поворот: неизвестный статус 0x%1").arg(status, 2, 16, QChar('0'));
}

FuncControlResult AMSProtocol::funcControlDetails(quint32 bitMask)
{
    // Таблица: {бит, описание, isFault}
        // isFault=true  → "Неисправности"
        // isFault=false → "Ошибки"
        static const struct {
            quint32    bit;
            const char *desc;
            bool       isFault;
        } kTable[] = {
            { FAILURE_EXCHANGE_TRANSMITTER,   "Ошибка обмена с УМ", false },
            { FAILURE_EXCHANGE_SCH        ,   "Ошибка обмена с СЧ", true  },
            { FAILURE_EXCHANGE_BEKU       ,   "Ошибка обмена с БЭКУ", false },
            { FAILURE_TRANSMITTER         ,   "Не готов УМ", false },
            { FAILURE_SCH                 ,   "Не готов СЧ", false },
            { FAILURE_BEKU                ,   "Не готов БЭКУ", true  },
            { FAILURE_BEKU_POWER          ,   "Неисправность модуля питания БЭКУ", true  },
            { FAILURE_BEKU_POWER_MHN      ,   "Отказ по питанию блока ЗМЛ", false },
            { FAILURE_BEKU_POWER_UM       ,   "Отказ по питанию УМ", false },
            { FAILURE_BEKU_POWER_SCH      ,   "Отказ по питанию СЧ", false },
            { FAILURE_BEKU_POWER_PM       ,   "Отказ по питанию ПМ", false },
            { FAILURE_BEKU_POWER_MSHU     ,   "Отказ по питанию МШУ", false },
            { ROTATION_TIMEOUT            ,   "Превышено время ожидания завершения вращения", true  },
            { ROTATION_FAILURE_ANGLE      ,   "Текущий угол не совпадает с заданным", true  },
            { ROTATION_EMERGENCY_STOP     ,   "Аварийная остановка привода вращения", true  },
            { ANGLE_SENSOR_FAILURE        ,   "Отказ датчика угла привода вращения", true  },
            { FAILURE_LITERA              ,   "Ошибка задания литеры", true  },
            { ANTENNA_EMERGENCY_STOP      ,   "Аварийная остановка открытия/закрытия антенны", true  },
            { ANTENNA_OPEN_TIMEOUT        ,   "Превышено время ожидания открытия антенны", true  },
            { ANTENNA_CLOSE_TIMEOUT       ,   "Превышено время ожидания закрытия антенны", true  },
            { NO_SOUNDING                 ,   "Нет сбора данных", true  },
            { FAILURE_STOPPER_LOCK        ,   "Ошибка блокировки стопора", false },
            { FAILURE_STOPPER_UNLOCK      ,   "Ошибка разблокировки стопора",  true  },
            { FAILURE_SOFTWARE_EXCHANGE   ,   "Ошибка обмена с БОУ", true  },
            { FAILURE_KV_OPEN_STATE       ,   "Ошибка сотсояния концевиков антенны РП", false },
            { FAILURE_KV_CLOSE_STATE      ,   "Ошибка состояния концевиков антенны ПП", false },
            { FAILURE_PILOT               ,   "Отказ при проверке пилот-синала", false },
            { FAILURE_TRANSMITTER_POWER   ,   "Отcутствует выходная импульная мощность", false },
            { SCH_MODE_SETTER_ERROR       ,   "Ошибка установки режимов в СЧ", false },
            { UM_MODE_SETTER_ERROR        ,   "Ошибка установки режимов в УМ", true  },
        };

        FuncControlResult result;
        for (const auto &e : kTable) {
            if (bitMask & e.bit) {
                if (e.isFault)
                    result.faults << QString::fromUtf8(e.desc);
                else
                    result.errors << QString::fromUtf8(e.desc);
            }
        }
        return result;
}

AMSCommand AMSProtocol::getPacketCommand(const QByteArray &data)
{
    if (data.isEmpty()) return static_cast<AMSCommand>(0x00);
    return static_cast<AMSCommand>(static_cast<quint8>(data[0]));
}

QVector<float> AMSProtocol::getAverageWindHeights(int count)
{
    // Генерация стандартных высот для среднего ветра
    // Логика из store/storetypes.h:214-237
    QVector<float> heights;
    heights.reserve(count);

    float currentHeight = 0.0f;
    int plus = 50;

    for (int i = 0; i < count; i++) {
        if (i == 0) {
            currentHeight = 50.0f;
        } else {
            if (currentHeight == 100.0f)
                plus = 100;
            else if (currentHeight == 200.0f)
                plus = 200;
            else if (currentHeight == 800.0f)
                plus = 400;
            else if (currentHeight == 2400.0f)
                plus = 200;
            else if (currentHeight == 2600.0f)
                plus = 400;
            else if (currentHeight == 3000.0f)
                plus = 1000;
            else if (currentHeight == 6000.0f)
                plus = 2000;

            currentHeight += plus;
        }
        heights.append(currentHeight);
    }

    return heights;
}

QVector<float> AMSProtocol::getActualWindHeights(int count)
{
    // Генерация стандартных высот для действительного ветра
    // Логика из store/storetypes.h:196-212
    QVector<float> heights;
    heights.reserve(count);

    float currentHeight = 0.0f;
    int plus = 25;

    for (int i = 0; i < count; i++) {
        if (i == 0) {
            currentHeight = 50.0f;
        } else {
            if (currentHeight == 100.0f)
                plus = 50;
            else if (currentHeight == 200.0f)
                plus = 100;
            else if (currentHeight == 1000.0f)
                plus = 200;

            currentHeight += plus;
        }
        heights.append(currentHeight);
    }

    return heights;
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

QByteArray AMSProtocol::createModeTransferPacket(WorkMode mode, AveragingTime avgTime, Litera litera)
{
    QByteArray packet;
    packet.append(static_cast<char>(CMD_MODE_TRANSFER));
    packet.append(static_cast<char>(mode));
    packet.append(static_cast<char>(avgTime));
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
//    QString dateTimeStr = currentDateTime.toString("MMddhhmmyyyy");
    QString dateTimeStr = currentDateTime.toString("MMddhhmmyyyy");
    packet.append(dateTimeStr.toLatin1());

    qDebug() << "Дата и время: " << dateTimeStr.toLatin1();

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
    QString dateTimeStr = dateTime.toString("MMddhhmmyyyy.ss");
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

    bitMask      = static_cast<quint32>(bytesToInt(data, 1));
    qDebug() << "BitMask: " << bitMask;
    powerOnCount = static_cast<quint32>(bytesToInt(data, 5));

    // Декодируем неисправности и логируем
    FuncControlResult fc = funcControlDetails(bitMask);
    if (fc.allOk()) {
        qInfo() << "AMSProtocol [0xA7]: Всё оборудование исправно";
    } else {
        for (const QString &f : fc.faults)
            qWarning() << "AMSProtocol [0xA7] Неисправность:" << f;
        for (const QString &e : fc.errors)
            qWarning() << "AMSProtocol [0xA7] Ошибка:" << e;
    }
    return true;
}

QVector<WindProfileData> AMSProtocol::parseAvgWindResponse(const QByteArray &data, bool &ok)
{
    QVector<WindProfileData> profile;
    ok = false;

    if (!isPacketValid(data)) return profile;
    if (getPacketCommand(data) != CMD_AVG_WIND_REQUEST) return profile;
    if (data.size() < 399) return profile; // 1 + 64 + 64 + 1 + 1

    // Получаем стандартные высоты для среднего ветра (16 уровней)
    QVector<float> heights = getAverageWindHeights(33);

    // 16 уровней высоты
    for (int i = 0; i < 33; i++) {
        WindProfileData point;
        point.height = bytesToFloat(data, 1 + 33 * 2 * 4 + i * 4);  // Устанавливаем стандартную высоту
        point.windDirection = (data, 1 + i * 4);
        point.windSpeed = bytesToFloat(data, 1 + 33 * 4 + i * 4);
//        point.windHeight = bytesToFloat(data, 1 + 33 * 2 * 4 + i * 4);
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
    if (data.size() < 399) return profile; // 1 + 120 + 120 + 1 + 1

    // Получаем стандартные высоты для действительного ветра (30 уровней)
    QVector<float> heights = getActualWindHeights(33);

    // 30 уровней высоты
    for (int i = 0; i < 33; i++) {
        WindProfileData point;
        point.height = bytesToFloat(data, 1 + 33 * 2 * 4 + i * 4);  // Устанавливаем стандартную высоту
        point.windDirection = bytesToFloat(data, 1 + i * 4);
        point.windSpeed = bytesToFloat(data, 1 + 33 * 4 + i * 4);
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
