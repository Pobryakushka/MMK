#ifndef AMSHANDLER_H
#define AMSHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "amsprotocol.h"

// Состояния процесса измерения
enum MeasurementStatus {
    STATUS_IDLE,      // Нет активных измерений
    STATUS_RUNNING,   // Измерение в процессе
    STATUS_READY,     // Измерение завершено успешно
    STATUS_FAILURE    // Ошибка измерения
};

// Этапы процесса измерения согласно диаграмме
enum MeasurementStage {
    STAGE_IDLE,                     // Начальное состояние
    STAGE_SEND_SOURCE_DATA,         // Отправка исходных данных
    STAGE_SEND_MODE,                // Передача признаков режимов (0xA1)
    STAGE_SEND_COORDS,              // Передача координат (0xA2)
    STAGE_SEND_DATETIME,            // Установка даты и времени (0xAE)
    STAGE_START_MEASUREMENT,        // Старт измерений (0xA3)
    STAGE_EXCHANGE_DATA,            // Обмен данными о процессе (0xA4)
    STAGE_REQUEST_RESULTS,          // Запрос результатов измерения
    STAGE_CALCULATE_PROFILE,        // Расчёт профиля ветра
    STAGE_COMPLETED                 // Завершение процесса
};

class AMSHandler : public QObject
{
    Q_OBJECT

public:
    explicit AMSHandler(QObject *parent = nullptr);
    ~AMSHandler();

    // Управление подключением
    bool connectToAMS(const QString &portName, qint32 baudRate,
                     QSerialPort::DataBits dataBits,
                     QSerialPort::Parity parity,
                     QSerialPort::StopBits stopBits);
    void disconnectFromAMS();
    bool isConnected() const;

    // Настройка БД
    void setDatabase(const QString &host, int port, const QString &dbName,
                    const QString &user, const QString &password);

    // Управление измерениями - новый подход согласно диаграмме
    bool startMeasurementSequence(WorkMode mode, AveragingTime avgTime, Litera litera,
                                 const StationCoordinates &coords,
                                 const QDateTime &dateTime);
    bool stopMeasurement();

    // Отправка исходных данных в процессе измерения
    bool sendSourceDataDuringMeasurement(int day, int hour, int tenMinutes,
                                        float stationAltitude,
                                        const QVector<float> &avgWindDir,
                                        const QVector<float> &avgWindSpeed,
                                        float reachedHeight,
                                        float surfaceWindDir, float surfaceWindSpeed,
                                        const QDateTime &currentDateTime);

    // Запрос данных (отдельные методы для совместимости)
    bool requestAvgWind();
    bool requestActualWind();
    bool requestMeasuredWind();
    bool requestFunctionalControl();

    // Установка параметров (отдельные методы для совместимости)
    bool setWorkMode(WorkMode mode, AveragingTime avgTime, Litera litera);
    bool setStationCoordinates(const StationCoordinates &coords);
    bool setDateTime(const QDateTime &dateTime);

    // Управление антенной
    bool openAntenna();
    bool closeAntenna();
    bool getAntennaStatus();
    bool rotateAntenna(float angle);
    bool stopAntennaRotation();

    // Получение текущего состояния
    MeasurementStatus getMeasurementStatus() const { return m_measurementStatus; }
    MeasurementStage getMeasurementStage() const { return m_currentStage; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void statusMessage(const QString &message);

    // Сигналы о полученных данных
    void measurementProgressUpdated(int percent, float angle);
    void avgWindDataReceived(const QVector<WindProfileData> &data);
    void actualWindDataReceived(const QVector<WindProfileData> &data);
    void measuredWindDataReceived(const QVector<MeasuredWindData> &data);
    void functionalControlDataReceived(quint32 bitMask, quint32 powerOnCount);
    void antennaStatusReceived(quint8 status);

    // Новые сигналы для этапов измерения
    void measurementStageChanged(MeasurementStage stage, const QString &description);
    void measurementStatusChanged(MeasurementStatus status);
    void measurementCompleted(int recordId);
    void measurementFailed(const QString &reason);

    // Сигнал для запроса промежуточных данных
    void needIntermediateData(int progress); // progress = 80 или 72

    // Сигналы о записи в БД
    void dataWrittenToDatabase(int recordId);
    void databaseError(const QString &error);

private slots:
    void onDataReceived();
    void onSerialError(QSerialPort::SerialPortError error);
    void onResponseTimeout();
    void onExchangeDataTimer(); // Новый таймер для опроса (0xA4)

private:
    QSerialPort *m_serialPort;
    AMSProtocol *m_protocol;
    QTimer *m_responseTimer;
    QTimer *m_exchangeDataTimer; // Таймер для периодического опроса
    QByteArray m_receiveBuffer;

    // Состояние
    bool m_waitingForResponse;
    bool m_isConnecting;
    AMSCommand m_lastCommand;
    int m_currentRecordId;

    // Новые переменные состояния для процесса измерения
    MeasurementStatus m_measurementStatus;
    MeasurementStage m_currentStage;
    WorkMode m_currentMode;
    AveragingTime m_currentAveragingTime;
    Litera m_currentLitera;
    StationCoordinates m_currentCoords;
    QDateTime m_currentDateTime;
    int m_lastProgress;
    float m_lastAngle;

    // Внутренние методы
    bool sendPacket(const QByteArray &packet, AMSCommand command);
    void processReceivedPacket(const QByteArray &packet);

    // Новые методы для управления процессом измерения
    void advanceMeasurementStage();
    void setMeasurementStage(MeasurementStage stage);
    void setMeasurementStatus(MeasurementStatus status);
    void handleMeasurementProgress(int progress);
    void processExchangeDataResponse(int progress, float angle);
    void requestMeasurementResults();
    void completeMeasurement();
    void failMeasurement(const QString &reason);

    // Методы записи в БД
    int createMainArchiveRecord(const QString &notes = QString());
    bool saveAvgWindProfile(int recordId, const QVector<WindProfileData> &data);
    bool saveActualWindProfile(int recordId, const QVector<WindProfileData> &data);
    bool saveMeasuredWindProfile(int recordId, const QVector<MeasuredWindData> &data);
    bool saveWindProfilesReferences(int recordId, int avgProfileId, int actualProfileId,
                                   int measuredProfileId, int shearProfileId);
    bool saveStationCoordinates(int recordId, const StationCoordinates &coords);
    bool saveCriticalMessage(int recordId, const QString &message,
                            const QString &severity);
};

#endif // AMSHANDLER_H
