#ifndef STATIONSTATETHREAD_H
#define STATIONSTATETHREAD_H

#include <QThread>
#include "transfermanager.h"

class StationStateThread : public QThread
{
    Q_OBJECT
public:
    explicit StationStateThread(TransferManager* manager, QObject *parent = nullptr);

    void requestToStop();

signals:
    void stationStateUpdated();

protected:
    void run() override;

private:
    bool m_stop {false};

    TransferManager* m_transferManager/* {nullptr}*/;
};

#endif // STATIONSTATETHREAD_H
