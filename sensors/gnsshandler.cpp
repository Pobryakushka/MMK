#include "gnsshandler.h"
#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

GNSSHandler::GNSSHandler(QObject *parent)
    : QObject(parent)
    , m_receiver(new ZedF9PReceiver(this))
    , m_hasValidFix(false)
{
    connect(m_receiver, &ZedF9PReceiver::dataReceived, this, &GNSSHandler::onReceiverData);
    connect(m_receiver, &ZedF9PReceiver::connected, this, &GNSSHandler::onReceiverConnected);
    connect(m_receiver, &ZedF9PReceiver::disconnected, this, &GNSSHandler::onReceiverDisconnected);
    connect(m_receiver, &ZedF9PReceiver::errorOccurred, this, &GNSSHandler::onReceiverError);
    connect(m_receiver, &ZedF9PReceiver::nmeaReceived, this, &GNSSHandler::onNmeaReceived);
}

GNSSHandler::~GNSSHandler()
{
    if (m_receiver->isConnected())
        m_receiver->disconnectFromReceiver();
}

bool GNSSHandler::connectToGnss(const QString &port, int baudRate)
{
    return m_receiver->connectToReceiver(port, baudRate);
}

void GNSSHandler::disconnectFromGnss()
{
    m_receiver->disconnectFromReceiver();
}

bool GNSSHandler::isConnected() const
{
    return m_receiver->isConnected();
}

bool GNSSHandler::updateCoordinatesInDb(int recordId)
{
    if (!m_hasValidFix) {
        QString err = "GNSSHandler: Нет валидного фикса - координаты в БД не обновлены";
        qWarning() << err;
        emit dbError(err);
        return false;
    }

    if (!DatabaseManager::instance()->isConnected()) {
        QString err = "GNSSHandler: БД не подключена";
        qWarning() << err;
        emit dbError(err);
        return false;
    }

    QSqlQuery query(DatabaseManager::instance()->database());
    query.prepare(
                "UPDATE station_coordinates "
                "SET latitude = :lat, longitude = :lon, altitude = :alt "
                "WHERE record_id = :record_id"
    );
    query.bindValue(":lat", m_lastData.latitude);
    query.bindValue(":lon", m_lastData.longitude);
    query.bindValue(":alt", m_lastData.altitude);
    query.bindValue(":record_id", recordId);

    if (!query.exec()) {
        QString err = QString("GNSSHandler: Ошибка обновления координат в БД: %1")
                .arg(query.lastError().text());
        qCritical() << err;
        emit dbError(err);
        return false;
    }

    if (query.numRowsAffected() == 0) {
        query.prepare(
                    "INSERT INTO station_coordinates (record_id, latitude, longitude, altitude) "
                    "VALUES (:record_id, :lat, :lon, :alt)"
        );
        query.bindValue(":record_id", recordId);
        query.bindValue(":lat", m_lastData.latitude);
        query.bindValue(":lon", m_lastData.longitude);
        query.bindValue(":alt", m_lastData.altitude);

        if (!query.exec()) {
            QString err = QString("GNSSHandler: Ошибка INSERT координат: %1")
                    .arg(query.lastError().text());
            qCritical() << err;
            emit dbError(err);
            return false;
        }
    }

    qInfo() << "GNSSHandler: Координаты GNSS обновлены в БД, record_id:" << recordId
            << "lat:" << m_lastData.latitude
            << "lon:" << m_lastData.longitude
            << "alt:" << m_lastData.altitude;

    emit coordinatesUpdated(recordId);
    return true;
}

void GNSSHandler::onReceiverData(const GNSSData &data)
{
    if (data.fixQuality != 0 &&
            !(data.latitude == 0.0 && data.longitude == 0.0))
    {
        m_lastData = data;
        m_hasValidFix = true;
    }
    emit dataReceived(data);
}

void GNSSHandler::onReceiverConnected()
{
    m_hasValidFix = false;
    emit connected();
}

void GNSSHandler::onReceiverDisconnected()
{
    m_hasValidFix = false;
    emit disconnected();
}

void GNSSHandler::onReceiverError(const QString &error) { emit errorOccurred(error); }
void GNSSHandler::onNmeaReceived(const QString &nmea) { emit nmeaReceived(nmea); }
