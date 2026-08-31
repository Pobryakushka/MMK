#ifndef CUSTOMPROTOCOL_H
#define CUSTOMPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QtGlobal>
#include <QDateTime>

// Структура данных пакета 0x2A
struct CustomNavigationData {
    double time;                    // Время
    quint16 coordinateSystem;       // Номер системы координат (1 - СК-42)
    quint16 projection;             // Проекция (0 - геодезическая)
    double latitude;                // Широта (рад)
    double longitude;               // Долгота (рад)
    double altitude;                // Высота (м)
    float umshv;                    // Разность между местной шкалой времени и шкалой ГЛОНАСС (сек)
    float v2d;                      // Горизонтальная составляющая вектора скорости (м/с)
    float az;                       // Азимут (град)
    float vh;                       // Вертикальная составляющая вектора скорости (м/с)
    float f;                        // Скорость ухода местной шкалы времени (сек/сек)
    float gps_glo;                  // Разность шкал времени GPS и ГЛОНАСС (сек)
    float dt;                       // Время измерений (мс)
    float pdop;                     // PDOP
    float vdop;                     // VDOP
    float hdop;                     // HDOP
    float tdop;                     // TDOP
    quint32 sat_glo;                // Номера спутников ГЛОНАСС
    quint32 sat_gps;                // Номера спутников GPS
    quint16 dispersion;             // Дисперсия (см. формат ниже)
    
    QDateTime timestamp;            // Временная метка получения данных
    
    // Методы для работы с полем дисперсии
    quint8 getQuality() const { return dispersion & 0x07; }
    quint8 getNumSatPosition() const { return (dispersion >> 3) & 0x0F; }
    bool isVolumetricPosition() const { return !((dispersion >> 7) & 0x01); }
    bool isVolumetricVelocity() const { return !((dispersion >> 8) & 0x01); }
    bool isDualSystem() const { return (dispersion >> 9) & 0x01; }
    bool hasPositionSolution() const { return (dispersion >> 10) & 0x01; }
    bool hasVelocitySolution() const { return (dispersion >> 11) & 0x01; }
    quint8 getNumSatVelocity() const { return (dispersion >> 12) & 0x0F; }
    
    QString getQualityString() const {
        switch (getQuality()) {
            case 0: return "< 3 м";
            case 1: return "< 30 м";
            case 2: return "< 100 м";
            case 3: return "< 300 м";
            case 4: return "> 300 м";
            default: return "Неизвестно";
        }
    }
};

class CustomProtocol : public QObject
{
    Q_OBJECT

public:
    explicit CustomProtocol(QObject *parent = nullptr);
    ~CustomProtocol();
    
    // Формирование запроса данных (пакет 0x2A)
    QByteArray buildRequestPacket();
    QByteArray buildDataPacket(const CustomNavigationData &navData);
    
    // Разбор полученного пакета
    bool parsePacket(const QByteArray &data, CustomNavigationData &navData);
    
    // Добавление данных в буфер для разбора
    void addData(const QByteArray &data);
    
signals:
    void navigationDataReceived(const CustomNavigationData &data);
    void parseError(const QString &error);
    
private:
    // Константы протокола
    static const quint8 PACKET_START = 0x20;
    static const quint8 FLAG = 0x10;
    static const quint8 PACKET_ID = 0x2A;
    static const quint8 END_OF_TEXT = 0x03;
    
    // Вспомогательные методы
    QByteArray escapeData(const QByteArray &data);
    QByteArray unescapeData(const QByteArray &data);
    quint16 calculateChecksum(const QByteArray &data);
    
    QByteArray encodeWord(quint16 value);
    QByteArray encodeDouble(double value);
    QByteArray encodeFloat(float value);
    QByteArray encodeUInt32(quint32 value);
    
    bool decodeWord(const QByteArray &data, int &offset, quint16 &value);
    bool decodeDouble(const QByteArray &data, int &offset, double &value);
    bool decodeFloat(const QByteArray &data, int &offset, float &value);
    bool decodeUInt32(const QByteArray &data, int &offset, quint32 &value);
    
    void processBuffer();
    
    QByteArray m_buffer;
};

#endif // CUSTOMPROTOCOL_H
