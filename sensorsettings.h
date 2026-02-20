#ifndef SENSORSETTINGS_H
#define SENSORSETTINGS_H

#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include "autoconnector.h"
#include <QSettings>

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

    // Получение настроек для БИНС
    QString getBinsComPort() const;
    int getBinsBaudRate() const;
    QSerialPort::DataBits getBinsDataBits() const;
    QSerialPort::Parity getBinsParity() const;
    QSerialPort::StopBits getBinsStopBits() const;

    // Установка статуса
    void setConnectionStatus(const QString& status, bool connected);
    void setConnectionEnabled(bool enabled);

    // Установка статуса АМС
    void setAmsConnectionStatus(const QString& status, bool connected);
    void setAmsConnectionEnabled(bool enabled);
    void setAmsConnectionError(const QString& errorMessage);

    // Установка статуса GNSS (независимо от вкладки)
    void setGnssConnectionStatus(const QString& status, bool connected);
    void setGnssConnectionEnabled(bool enabled);

    // Установка статуса ИВС (независимо от вкладки)
    void setIwsConnectionStatus(const QString& status, bool connected);
    void setIwsConnectionEnabled(bool enabled);

    // Установка статуса БИНС
    void setBinsConnectionStatus(const QString& status, bool connected);
    void setBinsConnectionEnabled(bool enabled);

    // Получение настроек для GNSS (независимо от активной вкладки)
    QString getGnssComPort() const;
    int getGnssBaudRate() const;
    QSerialPort::DataBits getGnssDataBits() const;
    QSerialPort::Parity getGnssParity() const;
    QSerialPort::StopBits getGnssStopBits() const;

    // Получение настроек для ИВС (независимо от активной вкладки)
    QString getIwsComPort() const;
    int getIwsBaudRate() const;
    QSerialPort::DataBits getIwsDataBits() const;
    QSerialPort::Parity getIwsParity() const;
    QSerialPort::StopBits getIwsStopBits() const;
    int getIwsProtocolIndex() const;
    quint8 getIwsDeviceAddress() const;
    int getIwsPollInterval() const;

signals:
    void connectRequested();
    void disconnectRequested();

    void gnssConnectRequested();
    void gnssDisconnectRequested();

    void amsConnectRequested();
    void amsDisconnectRequested();

    void binsConnectRequested();
    void binsDisconnectRequested();

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

    void onRefreshBinsPortsClicked();
    void onConnectBinsClicked();
    void onDisconnectBinsClicked();

    // AutoConnector слоты
    void onAutoConnectClicked();
    void onAutoDetectionStarted();
    void onAutoDetectionFinished();
    void onDeviceDetected(AutoConnector::DeviceType type, const QString &portName, int baudRate);
    void onAutoConnectProgress(int current, int total);
    void onAutoConnectLog(const QString &message);

private:
    Ui::SensorSettings *ui;
    AutoConnector *m_autoConnector;

    void populateComPorts();
    void populateGnssPorts();
    void populateAmsPorts();
    void populateBinsPorts();
    void setupConnections();
    void setComboBoxPort(QComboBox *comboBox, const QString &portName);

    void saveSettings();
    void loadSettings();
};

#endif // SENSORSETTINGS_H
