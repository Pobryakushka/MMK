#ifndef SENSORSETTINGS_H
#define SENSORSETTINGS_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

namespace Ui {
class SensorSettings;
}

class SensorSettings : public QDialog
{
    Q_OBJECT

public:
    explicit SensorSettings(QWidget *parent = nullptr);
    ~SensorSettings();

    // Получение настроек для ИВС/GNSS
    QString getComPort() const;
    int getBaudRate() const;
    QSerialPort::DataBits getDataBits() const;
    QSerialPort::Parity getParity() const;
    QSerialPort::StopBits getStopBits() const;
    int getProtocolIndex() const;
    quint8 getDeviceAddress() const;
    int getPollInterval() const;

    // Получение настроек для АМС
    QString getAmsComPort() const;
    int getAmsBaudRate() const;
    QSerialPort::DataBits getAmsDataBits() const;
    QSerialPort::Parity getAmsParity() const;
    QSerialPort::StopBits getAmsStopBits() const;

    // Установка статуса
    void setConnectionStatus(const QString& status, bool connected);
    void setConnectionEnabled(bool enabled);

    // Установка статуса АМС
    void setAmsConnectionStatus(const QString& status, bool connected);
    void setAmsConnectionEnabled(bool enabled);
    void setAmsConnectionError(const QString& errorMessage);

signals:
    void connectRequested();
    void disconnectRequested();

    void gnssConnectRequested();
    void gnssDisconnectRequested();

    void amsConnectRequested();
    void amsDisconnectRequested();

private slots:
    void onRefreshPortsClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onCloseClicked();

    void onRefreshGnssPortsClicked();
    void onConnectGnssClicked();
    void onDisconnectGnssClicked();

    void onRefreshAmsPortsClicked();
    void onConnectAmsClicked();
    void onDisconnectAmsClicked();

private:
    Ui::SensorSettings *ui;

    void populateComPorts();
    void populateGnssPorts();
    void populateAmsPorts();
    void setupConnections();
};

#endif // SENSORSETTINGS_H
