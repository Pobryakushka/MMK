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

    // Получение настроек
    QString getComPort() const;
    int getBaudRate() const;
    QSerialPort::DataBits getDataBits() const;
    QSerialPort::Parity getParity() const;
    QSerialPort::StopBits getStopBits() const;
    int getProtocolIndex() const;
    quint8 getDeviceAddress() const;
    int getPollInterval() const;

    // Установка статуса
    void setConnectionStatus(const QString& status, bool connected);
    void setConnectionEnabled(bool enabled);

signals:
    void connectRequested();
    void disconnectRequested();

private slots:
    void onRefreshPortsClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onCloseClicked();

    void onRefreshGnssPortsClicked();
    void onConnectGnssClicked();
    void onDisconnectGnssClicked();

private:
    Ui::SensorSettings *ui;

    void populateComPorts();
    void populateGnssPorts();
    void setupConnections();
};

#endif // SENSORSETTINGS_H
