#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include "cportmanager.h"
#include "cmsgprovider.h"
#include <ccomport.h>
#include "transfertypes.h"
#include <vector>
#include <QMutex>

class AbstractAnswerer;
class AbstractRequester;

//const int COMMANDS_1V16_COUNT = 16;

class TransferManager {
private:
    bool isStarted, inited;
    CPortManager* comManager1V16;
    std::vector<CComand*> commands1V16;
//    CComand* commands1V16[COMMANDS_1V16_COUNT];
    CMsgProvider* msgProvider1V16;
    CommandsForBUOIFrom1V16 cmdsFrom1V16;


    void del();
    void initClients();

public:
    CMsgProvider* msgProvider() const;

    TransferManager();
    virtual ~TransferManager();
    void init(CComPort *comport);
    bool start();
    void stop();
    AbstractAnswerer* answerer(unsigned char command);
    AbstractRequester* requester(unsigned char command);
    CommandsForBUOIFrom1V16& getCmdsFrom1V16() {
        return cmdsFrom1V16;
    }
};

#endif
