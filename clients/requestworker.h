#ifndef REQUESTSWORKER_H
#define REQUESTSWORKER_H

#include <QObject>
#include <QByteArray>
#include "transfermanager.h"

class RequestsWorker : public QObject
{
    Q_OBJECT
public:
    explicit RequestsWorker(TransferManager* manager, QObject* parent = nullptr)
        : QObject(parent)
        , m_manager(manager)
    {}

public slots:
    void doRequest(unsigned char command, const QByteArray &requestData = {});
    void doRequestTwo(unsigned char cmd1, unsigned char cmd2, const QByteArray &requestData = {});

signals:
    void requestFinished(unsigned char command, bool success, const QString &errorString);

private:
    TransferManager* m_manager = nullptr;
};

#endif // REQUESTSWORKER_H
