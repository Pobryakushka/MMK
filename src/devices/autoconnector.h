#ifndef AUTOCONNECTOR_H
#define AUTOCONNECTOR_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QMap>
#include <QSet>

/**
 * @brief Класс для автоматического определения и подключения датчиков
 *
 * Определяет тип устройства на каждом доступном COM-порту:
 * - АМС: отправляет команду LINE_TEST (0xA0), ждет ответ
 * - GNSS: слушает порт, ищет NMEA строки ($GNGGA, $GNRMC и т.д.)
 * - ИВС (UMB): отправляет UMB запрос, проверяет ответ
 * - БИНС: слушает порт, ищет пакеты 0xAA/0x02 с валидным CRC16 (устройство
 *   само непрерывно транслирует данные, порт открывается только на чтение —
 *   отправлять команду не нужно)
 */
class AutoConnector : public QObject
{
    Q_OBJECT

public:
    enum DeviceType {
        DEVICE_UNKNOWN = 0,
        DEVICE_AMS,
        DEVICE_GNSS,
        DEVICE_IWS,
        DEVICE_BINS
    };

    struct DeviceInfo {
        DeviceType type;
        QString portName;
        int baudRate;
        QString description;
    };

    explicit AutoConnector(QObject *parent = nullptr);
    ~AutoConnector();

    // Запуск автоопределения.
    // onlyType == DEVICE_UNKNOWN (по умолчанию) — искать все типы, как раньше.
    // onlyType == конкретный тип — искать ТОЛЬКО его (перебор всех портов,
    // но проверяется только один тип устройства на каждом порту).
    void startDetection(DeviceType onlyType = DEVICE_UNKNOWN);

    // Остановка процесса
    void stopDetection();

    // Получение результатов
    QMap<DeviceType, DeviceInfo> getDetectedDevices() const;

    // Проверка идет ли сейчас определение
    bool isDetecting() const { return m_isDetecting; }

    // Какой тип искали, если поиск был ограничен одним типом
    // (DEVICE_UNKNOWN, если шёл полный поиск по всем типам).
    DeviceType singleSearchTarget() const { return m_singleSearchTarget; }

signals:
    // Начало процесса определения
    void detectionStarted();

    // Завершение процесса определения
    void detectionFinished();

    // Найдено устройство
    void deviceDetected(DeviceType type, const QString &portName, int baudRate);

    // Прогресс (текущий порт / всего портов)
    void progressUpdated(int current, int total);

    // Логирование
    void logMessage(const QString &message);

    // Ошибки
    void errorOccurred(const QString &error);

private slots:
    void processNextPort();
    void onReadyRead();
    void onTimeout();

private:
    // Список портов для проверки
    QStringList m_portsToCheck;
    int m_currentPortIndex;

    // Текущий тестируемый порт
    QSerialPort *m_testPort;
    QTimer *m_timeoutTimer;
    QByteArray m_receiveBuffer;

    // Текущая фаза тестирования
    enum TestPhase {
        PHASE_AMS_TEST,
        PHASE_GNSS_LISTEN,
        PHASE_IWS_TEST,
        PHASE_BINS_LISTEN,
        PHASE_DONE
    };
    TestPhase m_currentPhase;

    // Результаты
    QMap<DeviceType, DeviceInfo> m_detectedDevices;
    bool m_isDetecting;

    // Типы, которые нужно ПРОПУСКАТЬ при переборе фаз — используется для
    // поиска только одного конкретного типа устройства: перед стартом сюда
    // предзаполняются все типы КРОМЕ искомого, и вся существующая логика
    // "пропускаем уже найденные типы" начинает работать как "пропускаем
    // все типы кроме искомого", без отдельного дублирования кода.
    // НЕ путать с m_detectedDevices — сюда никогда не попадают реальные
    // результаты поиска.
    QSet<DeviceType> m_skipTypes;
    DeviceType m_singleSearchTarget = DEVICE_UNKNOWN;

    // Временные данные для текущего порта
    QString m_currentPortName;
    int m_currentBaudRate;
    bool m_deviceFoundOnCurrentPort;

    // Методы тестирования
    void openPortAndTest(const QString &portName, int baudRate);
    void closeCurrentPort();
    void testAMS();
    void testGNSS();
    void testIWS();
    void testBINS();
    void moveToNextPhase();
    void deviceFound(DeviceType type, const QString &description);

    // true, если тип уже найден ИЛИ он не входит в число искомых при
    // ограниченном поиске одного типа.
    bool isTypeSkipped(DeviceType type) const;

    // Вспомогательные методы
    bool isAmsResponse(const QByteArray &data);
    bool isNmeaData(const QByteArray &data);
    bool isUmbResponse(const QByteArray &data);
    bool isBinsResponse(const QByteArray &data);

    QByteArray createAmsLineTestCommand();
    QByteArray createIwsTestCommand();

    // Вычисление контрольной суммы
    quint8 calculateChecksumAuto(const QByteArray &data);

    // Добавление контрольной суммы и стопового байта
    QByteArray finalizePacketAuto(const QByteArray &data);

    quint16 calculateUmbCrc(const QByteArray &data);

    // CRC16 (полином 0x1021, init 0xFFFF) — тот же алгоритм, что и в
    // BINSHandler::calculateCRC16, для проверки пакетов БИНС при автопоиске.
    quint16 calculateBinsCrc16(const QByteArray &data);

    int baudRateForPhase(TestPhase phase); // Скорость порта для каждой фазы
};

#endif // AUTOCONNECTOR_H
