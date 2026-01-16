#ifndef AMSHANDLER_H
#define AMSHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "amsprotocol.h"

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
    
    // Управление измерениями
    bool startMeasurement();
    bool stopMeasurement();
    bool continueMeasurement();
    
    // Запрос данных
    bool requestAvgWind();
    bool requestActualWind();
    bool requestMeasuredWind();
    bool requestFunctionalControl();
    
    // Установка параметров
    bool setWorkMode(WorkMode mode, Litera litera);
    bool setStationCoordinates(const StationCoordinates &coords);
    bool setDateTime(const QDateTime &dateTime);
    bool sendSourceData(int day, int hour, int tenMinutes,
                       float stationAltitude,
                       const QVector<float> &avgWindDir,
                       const QVector<float> &avgWindSpeed,
                       float reachedHeight,
                       float surfaceWindDir, float surfaceWindSpeed,
                       const QDateTime &currentDateTime);
    
    // Управление антенной
    bool openAntenna();
    bool closeAntenna();
    bool getAntennaStatus();
    bool rotateAntenna(float angle);
    bool stopAntennaRotation();
    
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
    
    // Сигналы о записи в БД
    void dataWrittenToDatabase(int recordId);
    void databaseError(const QString &error);

private slots:
    void onDataReceived();
    void onSerialError(QSerialPort::SerialPortError error);
    void onResponseTimeout();

private:
    QSerialPort *m_serialPort;
    AMSProtocol *m_protocol;
    QTimer *m_responseTimer;
    QByteArray m_receiveBuffer;
    
    // Состояние
    bool m_waitingForResponse;
    AMSCommand m_lastCommand;
    int m_currentRecordId;
    
    // Внутренние методы
    bool sendPacket(const QByteArray &packet, AMSCommand command);
    void processReceivedPacket(const QByteArray &packet);
    
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
