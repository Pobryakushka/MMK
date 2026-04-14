#ifndef BINSHANDLER_H
#define BINSHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>

// Структура данных БИНС
struct BINSData {
    double heading;
    double roll;
    double pitch;
    quint16 packetCounter;
//    quint32 timestamp;
    bool valid;

    BINSData()
        : heading(0.0), roll(0.0), pitch(0.0)
        , packetCounter(0), valid(false) {}
};

// Обработчик протокола БИНС
// Принимает бинарные пакеты по протоколу через RS-232
class BINSHandler : public QObject
{
    Q_OBJECT

public:
    explicit BINSHandler(QObject *parent = nullptr);
    ~BINSHandler();

    // Управление подключением
    bool connectToBINS(const QString &portName,
                       quint32 baudRate = 115200,
                       QSerialPort::DataBits dataBits = QSerialPort::Data8,
                       QSerialPort::Parity parity = QSerialPort::NoParity,
                       QSerialPort::StopBits stopBits = QSerialPort::OneStop);
    void disconnectFromBINS();
    bool isConnected() const;

    // Получение текущих данных
    BINSData getCurrentData() const { return m_currentData; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void dataReceived(const BINSData &data);
    void statusMessage(const QString &message);

private slots:
    void onSerialDataReceived();
    void onSerialError(QSerialPort::SerialPortError error);
    void onConnectionTimeout();

private:
    QSerialPort *m_serialPort;
    QTimer *m_connectionTimer;
    QByteArray m_receiveBuffer;
    BINSData m_currentData;

    // Константы протокола
    static const quint8 PACKET_HEADER = 0xAA;
    static const quint8 PACKET_TYPE_ORIENT = 0x02;
    static const int MIN_PACKET_SIZE = 18;
    static const int DATA_SIZE = 16;
    static const int CONNECTION_TIMEOUT = 5000;

    // Обработка пакетов
    void processReceivedData();
    bool parseOrientationPacket(const QByteArray &packet);

    // Вспомогательные функции
    quint16 calculateCRC16(const QByteArray &data);
    float extractFloat(const QByteArray &data, int offset);
    double extractDouble(const QByteArray &data, int offset);
    quint16 extractUInt16(const QByteArray &data, int offset);
    quint32 extractUInt32(const QByteArray &data, int offset);
};

#endif // BINSHANDLER_H
