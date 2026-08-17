#include "databasemanager.h"
#include <QDebug>
#include <QUuid>

DatabaseManager* DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
    , m_dbPort(5432)
    , m_configured(false)
{
    m_connectionName = "MainDBConnection" + QUuid::createUuid().toString();
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

DatabaseManager* DatabaseManager::instance()
{
if (!m_instance){
    m_instance = new DatabaseManager();
}
return m_instance;
}

void DatabaseManager::configure(const QString &host, int port, const QString &dbName, const QString &user, const QString &password)

{
    m_dbHost = host;
    m_dbPort = port;
    m_dbName = dbName;
    m_dbUser = user;
    m_dbPassword = password;
    m_configured = true;

    qInfo() << "DatabaseManager: Настроена БД" << dbName << "на" << host << ":" << port;
}

bool DatabaseManager::connect()
{
    if (!m_configured){
        qWarning() << "DatabaseManager: БД не настроена";
        emit error("БД не настроена");
        return false;
    }

    if (m_database.isOpen()){
        return true;
    }

    m_database = QSqlDatabase::addDatabase("QPSQL", m_connectionName);
    m_database.setHostName(m_dbHost);
    m_database.setPort(m_dbPort);
    m_database.setDatabaseName(m_dbName);
    m_database.setUserName(m_dbUser);
    m_database.setPassword(m_dbPassword);

    if (!m_database.open()){
        QString errorText = QString("Ошибка подключения к БД: %1").arg(m_database.lastError().text());
        qCritical() << "DatabaseManager:" << errorText;
        emit error(errorText);
        return false;
    }

    qInfo() << "DatabaseManager: Подключено к БД";
    emit connected();
    return true;
}

void DatabaseManager::disconnect()
{
    if (m_database.isOpen()){

        m_database.close();
        qInfo() << "DatabaseManager: Отключено от БД";
        emit disconnected();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DatabaseManager::isConnected() const
{
    return m_database.isOpen();
}
