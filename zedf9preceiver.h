#ifndef ZEDF9PRECEIVER_H
#define ZEDF9PRECEIVER_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>
#include <QDateTime>
#include "customprotocol.h"

struct GNSSData {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double speed = 0.0;
    double heading = 0.0;
    int satellites = 0;
    int fixQuality = 0;
    double hdop = 0.0;
    QDateTime timestamp;
    QString fixType;

    // RTX специфичные данные
    bool rtkFixed = false;
    bool rtkFloat = false;
    double accuracyH = 0.0;
    double accuracyV = 0.0;
};

struct UBXMessage {
    quint8 msgClass;
    quint8 msgID;
    QByteArray payload;
};

class ZedF9PReceiver : public QObject
{
    Q_OBJECT

public:
    explicit ZedF9PReceiver(QObject *parent = nullptr);
    ~ZedF9PReceiver();

    // Управление подключением
    bool connectToReceiver(const QString &portName, qint32 baudRate = 9600);
    void disconnectFromReceiver();
    bool isConnected() const;

    bool connectOutputPort(const QString &portName, qint32 baudRate = 9600);
    void disconnectOutputPort();
    bool isOutputPortConnected() const;

    static QStringList getAvailablePorts();

    // Конфигурация приемника
    bool configureUBXProtocol();
    bool enableNMEAMessages(bool enable);
    bool enableUBXMessages(bool enable);
    bool setNavigationRate(quint16 measRate = 1000);
    bool enableRTCM3(bool enable);

    bool setRTKMode(bool base = false);
    bool sendRTCMCorrections(const QByteArray &rtcmData);

    GNSSData getCurrentData() const { return m_currentData; }
    QString getLastNMEA() const { return m_lastNMEA; }

    bool saveConfiguration();
    bool resetToDefaults();

    bool sendCustomData(const CustomNavigationData &data);

signals:
    void connected();
    void disconnected();
    void outputPortConnected();
    void outputPortDisconnected();
    void errorOccurred(const QString &error);
    void dataReceived(const GNSSData &data);
    void nmeaReceived(const QString &nmea);
    void ubxReceived(const UBXMessage &msg);
    void rtcmReceived(const QByteArray &rtcm);
    void customDataReceived(const CustomNavigationData &data);

private slots:
    void onReadyRead();
    void onOutputPortReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onOutputPortErrorOccurred(QSerialPort::SerialPortError error);

private:
    // Парсинг протоколов
    void parseNMEA(const QString &sentence);
    void parseUBX(const QByteArray &data);
    void parseRTCM3(const QByteArray &data);

    // Обработка NMEA сообщений
    void parseGGA(const QStringList &fields);
    void parseRMC(const QStringList &fields);
    void parseGSA(const QStringList &fields);
    void parseGSV(const QStringList &fields);
    void parseVTG(const QStringList &fields);

    // Обработка UBX сообщений
    void parseUBX_NAV_PVT(const QByteArray &payload);
    void parseUBX_NAV_HPPOSLLH(const QByteArray &payload);
    void parseUBX_NAV_STATUS(const QByteArray &payload);

    bool sendUBXCommand(quint8 msgClass, quint8 msgID, const QByteArray &payload = QByteArray());
    QByteArray buildUBXMessage(quint8 msgClass, quint8 msgID, const QByteArray &payload);
    void calculateUBXChecksum(const QByteArray &data, quint8 &ckA, quint8 &ckB);

    bool validateNMEAChecksum(const QString &sentence);
    double nmeaToDecimal(const QString &coord, const QString &direction);

private:
    QSerialPort *m_serialPort;
    QSerialPort *m_outputPort;
    QByteArray m_buffer;
    QByteArray m_outputBuffer;
    GNSSData m_currentData;
    QString m_lastNMEA;

    QByteArray m_ubxBuffer;
    QByteArray m_rtcmBuffer;

    CustomProtocol *m_customProtocol;
};

#endif // ZEDF9PRECEIVER_H
