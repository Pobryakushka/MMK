#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>


class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager* instance();

    void configure(const QString &host, int port, const QString &dbName,
                   const QString &user, const QString &password);

    bool connect();
    void disconnect();
    bool isConnected() const;

    QSqlDatabase database() const { return m_database; }

signals:
    void connected();
    void disconnected();
    void error(const QString &errorText);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    static DatabaseManager *m_instance;

    QSqlDatabase m_database;
    QString m_dbHost;
    int m_dbPort;
    QString m_dbName;
    QString m_dbUser;
    QString m_dbPassword;
    bool m_configured;

    QString m_connectionName;
};

#endif // DATABASEMANAGER_H
