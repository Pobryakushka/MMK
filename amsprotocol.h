#ifndef AMSPROTOCOL_H
#define AMSPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QDateTime>
#include <QVector>

// Структура для профиля ветра (средний, действительный, измеренный)
struct WindProfileData {
    float height;           // Высота, м
    float windSpeed;        // Скорость ветра, м/с
    int windDirection;      // Направление ветра, градусы
    bool isValid;
    
    WindProfileData() : height(0), windSpeed(0), windDirection(0), isValid(false) {}
};

// Структура для измеренного ветра с признаком достоверности
struct MeasuredWindData {
    float height;           // Высота, м
    float windSpeed;        // Скорость ветра, м/с
    int windDirection;      // Направление ветра, градусы
    int reliability;        // Достоверность: 1 - недействительное, 2 - действительное
    
    MeasuredWindData() : height(0), windSpeed(0), windDirection(0), reliability(0) {}
};

// Структура данных о процессе измерений
struct MeasurementProgress {
    int percentComplete;    // -2: ошибка, -1: завершено, 0-100: процесс
    float currentAngle;     // Текущий угол РПВ, градусы
};

// Структура координат станции
struct StationCoordinates {
    int latitude;           // Широта в секундах
    int longitude;          // Долгота в секундах
    float altitude;         // Высота над уровнем моря, м
    float azimuth;          // Азимут, градусы
    float pitch;            // Угол тангажа, градусы
    float roll;             // Угол крена, градусы
    
    StationCoordinates() : latitude(0), longitude(0), altitude(0), 
                          azimuth(0), pitch(0), roll(0) {}
};

// Коды команд протокола АМС
enum AMSCommand : quint8 {
    CMD_LINE_TEST = 0xA0,           // Тест линии
    CMD_MODE_TRANSFER = 0xA1,       // Передача признаков режимов
    CMD_COORDS_TRANSFER = 0xA2,     // Передача координат
    CMD_START_MEASUREMENT = 0xA3,   // Старт измерений
    CMD_DATA_EXCHANGE = 0xA4,       // Обмен данными о процессе
    CMD_SOURCE_DATA = 0xA6,         // Передача исходных данных
    CMD_FUNC_CONTROL = 0xA7,        // Функциональный контроль
    CMD_AVG_WIND_REQUEST = 0xA9,    // Запрос среднего ветра
    CMD_ACTUAL_WIND_REQUEST = 0xAA, // Запрос действительного ветра
    CMD_TO2_MODE = 0xAB,            // Режим ТО-2
    CMD_MEASURED_WIND_REQUEST = 0xAC, // Запрос измеренного ветра
    CMD_ANTENNA_CONTROL = 0xAD,     // Открытие/закрытие антенны
    CMD_SET_DATETIME = 0xAE,        // Установка даты и времени
    CMD_ROTATE_ANTENNA = 0xAF       // Поворот антенны
};

// Типы режимов работы
enum WorkMode : quint8 {
    MODE_WORKING = 0x00,      // РАБОЧИЙ
    MODE_CALIBRATION = 0x02,  // КАЛИБРОВКА
    MODE_STANDBY = 0x05       // ДЕЖУРНЫЙ
};

// Литеры
enum Litera : quint8 {
    LITERA_1 = 0x00,
    LITERA_2 = 0x01,
    LITERA_3 = 0x02
};

class AMSProtocol : public QObject
{
    Q_OBJECT

public:
    explicit AMSProtocol(QObject *parent = nullptr);
    
    // Формирование пакетов для передачи
    QByteArray createLineTestPacket();
    QByteArray createModeTransferPacket(WorkMode mode, Litera litera);
    QByteArray createCoordsTransferPacket(const StationCoordinates &coords);
    QByteArray createStartMeasurementPacket();
    QByteArray createDataExchangePacket(bool continueProcess);
    QByteArray createSourceDataPacket(int day, int hour, int tenMinutes, 
                                     float stationAltitude, 
                                     const QVector<float> &avgWindDir,
                                     const QVector<float> &avgWindSpeed,
                                     float reachedHeight,
                                     float surfaceWindDir, float surfaceWindSpeed,
                                     const QDateTime &currentDateTime);
    QByteArray createFuncControlPacket();
    QByteArray createAvgWindRequestPacket();
    QByteArray createActualWindRequestPacket();
    QByteArray createMeasuredWindRequestPacket();
    QByteArray createAntennaControlPacket(quint8 command);
    QByteArray createSetDateTimePacket(const QDateTime &dateTime);
    QByteArray createRotateAntennaPacket(quint8 command, float angle);
    
    // Парсинг принятых пакетов
    bool parseLineTestResponse(const QByteArray &data);
    bool parseModeTransferResponse(const QByteArray &data);
    bool parseCoordsTransferResponse(const QByteArray &data);
    WorkMode parseStartMeasurementResponse(const QByteArray &data, bool &ok);
    MeasurementProgress parseDataExchangeResponse(const QByteArray &data, bool &ok);
    bool parseSourceDataResponse(const QByteArray &data);
    bool parseFuncControlResponse(const QByteArray &data, quint32 &bitMask, quint32 &powerOnCount);
    QVector<WindProfileData> parseAvgWindResponse(const QByteArray &data, bool &ok);
    QVector<WindProfileData> parseActualWindResponse(const QByteArray &data, bool &ok);
    QVector<MeasuredWindData> parseMeasuredWindResponse(const QByteArray &data, bool &ok);
    quint8 parseAntennaControlResponse(const QByteArray &data, bool &ok);
    bool parseSetDateTimeResponse(const QByteArray &data);
    bool parseRotateAntennaResponse(const QByteArray &data, quint8 &status, float &currentAngle);
    
    // Проверка пакета
    bool isPacketValid(const QByteArray &data);
    AMSCommand getPacketCommand(const QByteArray &data);
    
private:
    // Вычисление контрольной суммы
    quint8 calculateChecksum(const QByteArray &data);
    
    // Добавление контрольной суммы и стопового байта
    QByteArray finalizePacket(const QByteArray &data);
    
    // Преобразование float в QByteArray (little-endian)
    QByteArray floatToBytes(float value);
    
    // Преобразование int в QByteArray (little-endian)
    QByteArray intToBytes(qint32 value);
    
    // Чтение float из QByteArray
    float bytesToFloat(const QByteArray &data, int offset);
    
    // Чтение int из QByteArray
    qint32 bytesToInt(const QByteArray &data, int offset);
};

#endif // AMSPROTOCOL_H