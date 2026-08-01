#ifndef STATIONSTATETHREAD_CPP
#define STATIONSTATETHREAD_CPP

#include "stationstatethread.h"
#include "clients/abstractrequester.h"
#include <QDebug>


StationStateThread::StationStateThread(TransferManager* manager, QObject *parent)
    : QThread(parent),
      m_transferManager(manager)
{
}

void StationStateThread::requestToStop()
{
    m_stop = true;
}

void StationStateThread::run()
{
    while (!m_stop)
    {
        if (m_transferManager){

            auto ar = m_transferManager->requester(COMMAND_1V16_STATION_STATE);
            if (ar) {
                auto result = ar->sendRequest();
                if (result == AbstractRequester::Ok){
                    emit stationStateUpdated();
                } else{
                    QThread::msleep(100);
                    qDebug() << "[StationStateThread] Error: sendRequest timed out or init error";
                }
            }
        }
        QThread::sleep(3);
    }
}
#endif // STATIONSTATETHREAD_CPP
