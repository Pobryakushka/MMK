#ifndef GROUNDMETEOPARAMS_H
#define GROUNDMETEOPARAMS_H

#include <QDialog>
#include <QMap>
#include <QSerialPort>

namespace Ui {
class GroundMeteoParams;
}

class GroundMeteoParams : public QDialog {
    Q_OBJECT

public:
    enum RS485Protocol {
        MODBUS_RTU,
        UMB_PROTOCOL
    };

    explicit GroundMeteoParams(QWidget *parent = nullptr);
    ~GroundMeteoParams();

    void deleteDataFromTable();

    // Статический метод для получения экземпляра (синглтон паттерн)
    static GroundMeteoParams* instance();

    // RS485 функционал
    void setProtocol(RS485Protocol protocol);
    void setDeviceAddress(quint8 address);

    // Создание запросов (публичные методы)
    QByteArray createModbusReadRequest(const QList<quint16>& parameters);
    QByteArray createUmbReadRequest(const QList<quint16>& parameters);
    QByteArray createRequest(const QList<quint16>& parameters);

public slots:
    // Обработка входящих данных
    void onDataReceived(const QByteArray& data);

private slots:
    void updateTableWithData(const QMap<QString, double>& values);

private:
    Ui::GroundMeteoParams *ui;
    RS485Protocol m_protocol;
    quint8 m_deviceAddress;
    QByteArray m_receiveBuffer;

    // Статическая переменная для синглтона
    static GroundMeteoParams* s_instance;

    // Для отслеживания запрошенных регистров (Modbus)
    QList<quint16> m_lastRequestedRegisters;

    // Парсинг ответов (приватные методы)
    bool parseModbusResponse(const QByteArray& response, QMap<QString, double>& values);
    bool parseUmbResponse(const QByteArray& response, QMap<QString, double>& values);

    // НОВЫЕ методы для Modbus RTU с маппингом регистров
    bool parseModbusResponseWithMapping(
        const QByteArray& response,
        const QList<quint16>& requestedRegisters,
        QMap<QString, double>& values);

    bool convertModbusRegisterToValue(
        quint16 regAddr,
        quint16 rawValue,
        QString& paramName,
        double& scaledValue);

    // Вспомогательные функции
    quint16 calculateCRC16(const QByteArray& data);
    quint16 calculateModbusCRC16(const QByteArray& data);
    QString parameterCodeToName(quint16 code);
    QString mapParameterToTableRow(const QString& paramName);

signals:
    void errorOccurred(const QString& error);
    void dataUpdated(const QMap<QString, double>& values);
};

#endif // GROUNDMETEOPARAMS_H
