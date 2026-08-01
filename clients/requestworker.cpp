#include "clients/requestworker.h"
#include "abstractrequester.h"
#include "qdebug.h"

void RequestsWorker::doRequest(unsigned char command, const QByteArray &requestData)
{
    qDebug() << "doRequest -" << command;
    if(!m_manager) {
        emit requestFinished(command, false, QStringLiteral("TransferManager is null"));
        return;
    }

    AbstractRequester* ar = m_manager->requester(command);
    if(!ar) {
        emit requestFinished(command, false, QStringLiteral("No requester for this command"));
        return;
    }

    AbstractRequester::Errors err;
    if (!requestData.isEmpty()) {
        err = ar->sendRequest(reinterpret_cast<const unsigned char*>(requestData.constData()),
                              requestData.size());
    } else {
        err = ar->sendRequest();
    }

    if(err == AbstractRequester::Ok) {
        emit requestFinished(command, true, {});
    } else {
        QString msg;
        if (err == AbstractRequester::ErrorTimeOut)
            msg = "Timeout";
        else if (err == AbstractRequester::ErrorInit)
            msg = "Init error";
        else
            msg = "Unknown error";
        emit requestFinished(command, false, msg);
    }
}

void RequestsWorker::doRequestTwo(unsigned char cmd1, unsigned char cmd2, const QByteArray &requestData){
    if(!m_manager){
        emit requestFinished(cmd1, false, "TransferManager is null");
        return;
    }

        AbstractRequester* ar1 = m_manager->requester(cmd1);
        if(!ar1){
            emit requestFinished(cmd1, false, "Requester not found (cmd1)");
            return;
        }
        auto err1 = ar1->sendRequest();
        if (err1 != AbstractRequester::Ok){
            emit requestFinished(cmd1, false, "Error from cmd1");
            return;
        }
    AbstractRequester* ar2 = m_manager->requester(cmd2);
    if(!ar2){
        emit requestFinished(cmd2, false, "Requester not found (cmd2)");
        return;
    }
    AbstractRequester::Errors err2;
    if (!requestData.isEmpty()){
        err2 = ar2->sendRequest(reinterpret_cast<const unsigned char*>(requestData.constData()),
                                requestData.size());
    } else {
        err2 = ar2->sendRequest();
    }
   if (err2 != AbstractRequester::Ok){
       emit requestFinished(cmd2, false, "Error on cmd2");
       return;
    }
emit requestFinished(cmd2, true, {});
}
