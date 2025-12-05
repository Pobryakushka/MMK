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

    // RS485 функционал
    void setSerialPort(QSerialPort* port);
    void setProtocol(RS485Protocol protocol);
    void setDeviceAddress(quint8 address);

    // Отправка запроса данных
    void requestData(const QList<quint16>& parameters);

public slots:
    // Обработка входящих данных
    void onDataReceived(const QByteArray& data);

private slots:
    void updateTableWithData(const QMap<QString, double>& values);

private:
    Ui::GroundMeteoParams *ui;
    QSerialPort* m_serialPort;
    RS485Protocol m_protocol;
    quint8 m_deviceAddress;
    QByteArray m_receiveBuffer;

    // Создание запросов
    QByteArray createModbusReadRequest(const QList<quint16>& parameters);
    QByteArray createUmbReadRequest(const QList<quint16>& parameters);

    // Парсинг ответов
    bool parseModbusResponse(const QByteArray& response, QMap<QString, double>& values);
    bool parseUmbResponse(const QByteArray& response, QMap<QString, double>& values);

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
