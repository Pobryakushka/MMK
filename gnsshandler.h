#ifndef GNSSHANDLER_H
#define GNSSHANDLER_H

#include <QObject>
#include "zedf9preceiver.h"

class GNSSHandler : public QObject
{
    Q_OBJECT

public:
    explicit GNSSHandler(QObject *parent = nullptr);
    ~GNSSHandler();

    bool connectToGnss(const QString &port, int baudRate);
    void disconnectFromGnss();
    bool isConnected() const;

    GNSSData getCurrentData() const { return m_lastData; }
    bool hasVaidFix() const { return m_hasValidFix; }

    // Вызывается из MainWindow::onAmsMeasurementCompleted()
    // Обновляет запись station_coordinates для данного record_id
    bool updateCoordinatesInDb(int recordId);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void dataReceived(const GNSSData &data);
    void nmeaReceived(const QString &nmea);
    void coordinatesUpdated(int recordId);
    void dbError(const QString &error);

private slots:
    void onReceiverData(const GNSSData &data);
    void onReceiverConnected();
    void onReceiverDisconnected();
    void onReceiverError(const QString &error);
    void onNmeaReceived(const QString &nmea);

private:
    ZedF9PReceiver *m_receiver;
    GNSSData m_lastData;
    bool m_hasValidFix;
};

#endif // GNSSHANDLER_H
