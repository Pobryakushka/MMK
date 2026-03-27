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
    float windHeight;       // Высота, м
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

// Причина отказа разбора пакета
enum ParseError {
    PARSE_OK,                   // Успешно
    PARSE_ERR_TOO_SHORT,        // Пакет короче минимального
    PARSE_ERR_BAD_STOP,         // Стоповый байт не 0xFF
    PARSE_ERR_BAD_CHECKSUM,     // Контрольная сумма не совпадает
    PARSE_ERR_WRONG_COMMAND,    // Команда в ответе не совпадает с ожидаемой
    PARSE_ERR_BAD_DATA          // Данные пакета некорректны
};

// Статус антенны (команда 0xAD)
enum AntennaStatus : quint8 {
    ANTENNA_IN_PROGRESS = 0x00, // Процесс открытия/закрытия
    ANTENNA_SUCCESS     = 0x01, // Завершено успешно
    ANTENNA_FAULT       = 0x02  // Аварийная остановка
};

// Статус поворота антенны (команда 0xAF)
enum RotateStatus : quint8 {
    ROTATE_IDLE_OK  = 0x00,     // Ожидание / завершено успешно
    ROTATE_RUNNING  = 0x01,     // Вращение
    ROTATE_FAULT    = 0x02      // Аварийная остановка
};

// Биты неисправностей из функционального контроля (0xA7, Таблица 2)
// Бит=1 → устройство неисправно, бит=0 → устройство исправно
enum FuncControlBit : quint32 {
    FAILURE_EXCHANGE_TRANSMITTER = (1u << 0), // Бит 1: Превышено время ожидания вращения
    FAILURE_EXCHANGE_SCH         = (1u << 1), // Бит 2: Аварийная остановка открытия/закрытия антенны
    FAILURE_EXCHANGE_BEKU        = (1u << 2), // Бит 3: Превышено время ожидания открытия антенны
    FAILURE_TRANSMITTER          = (1u << 3), // Бит 4: Превышено время ожидания закрытия антенны
    FAILURE_SCH                  = (1u << 4), // Бит 5: Нет сбора данных
    FAILURE_BEKU                 = (1u << 5), // Бит 6: СЧ не пошёл контроль
    FAILURE_BEKU_POWER           = (1u << 6), // Бит 7: Не готов передатчик
    FAILURE_BEKU_POWER_MHN       = (1u << 7), // Бит 8: Ошибка ПО
    FAILURE_BEKU_POWER_UM        = (1u << 8), // Бит 9: Неверное значение даты и времени
    FAILURE_BEKU_POWER_SCH       = (1u <<  9), // Ошибка обмена с УМ
    FAILURE_BEKU_POWER_PM        = (1u << 10), // Ошибка обмена с СЧ
    FAILURE_BEKU_POWER_MSHU      = (1u << 11), // Ошибка обмена с БЭКУ
    ROTATION_TIMEOUT             = (1u << 12), // Не готов УМ
    ROTATION_FAILURE_ANGLE       = (1u << 13), // Не готов СЧ
    ROTATION_EMERGENCY_STOP      = (1u << 14), // Не готов БЭКУ
    ANGLE_SENSOR_FAILURE         = (1u << 15), // Неисправность модуля питания БЭКУ
    FAILURE_LITERA               = (1u << 16), // Отказ по питанию блока ЗМЛ
    ANTENNA_EMERGENCY_STOP       = (1u << 17), // Отказ по питанию УМ
    ANTENNA_OPEN_TIMEOUT         = (1u << 18), // Отказ по питанию СЧ
    ANTENNA_CLOSE_TIMEOUT        = (1u << 19), // Отказ по питанию ПМ
    NO_SOUNDING                  = (1u << 20), // Отказ по питанию МШУ
    FAILURE_STOPPER_LOCK         = (1u << 21), // Текущий угол не совпадает с заданным
    FAILURE_STOPPER_UNLOCK       = (1u << 22), // Аварийная остановка привода вращения
    FAILURE_SOFTWARE_EXCHANGE    = (1u << 23), // Отказ датчика угла привода вращения
    FAILURE_KV_OPEN_STATE        = (1u << 24), // Ошибка задания литеры
    FAILURE_KV_CLOSE_STATE       = (1u << 25), // Ошибка блокировки стопора
    FAILURE_PILOT                = (1u << 26), // Ошибка разблокировки стопора
    FAILURE_TRANSMITTER_POWER    = (1u << 27), // Ошибка состояния концевиков антенны РП
    SCH_MODE_SETTER_ERROR        = (1u << 28), // Ошибка состояния концевиков антенны ПП
    UM_MODE_SETTER_ERROR         = (1u << 29), // Отказ при проверке пилот-сигнала
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

// Время усреднения
enum AveragingTime : quint8 {
    AVERAGING_3_MIN = 0x01,   // 3 минуты
    AVERAGING_6_MIN = 0x02,   // 6 минут
    AVERAGING_9_MIN = 0x03    // 9 минут
};

struct FuncControlResult {
    QStringList faults;
    QStringList errors;
    bool allOk() const { return faults.isEmpty() && errors.isEmpty(); }
};

class AMSProtocol : public QObject
{
    Q_OBJECT

public:
    explicit AMSProtocol(QObject *parent = nullptr);

    // Формирование пакетов для передачи
    QByteArray createLineTestPacket();
    QByteArray createModeTransferPacket(WorkMode mode, AveragingTime avgTime, Litera litera);
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

    // Детальная диагностика ошибок
    ParseError checkPacket(const QByteArray &data, AMSCommand expectedCmd, int minSize);
    static QString parseErrorString(ParseError err);
    static QString antennaStatusString(quint8 status);
    static QString rotateStatusString(quint8 status);
    static FuncControlResult funcControlDetails(quint32 bitMask);

    // Генерация стандартных высот для профилей ветра (публичные статические методы)
    static QVector<float> getAverageWindHeights(int count = 16);    // Высоты для среднего ветра
    static QVector<float> getActualWindHeights(int count = 30);     // Высоты для действительного ветра

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
