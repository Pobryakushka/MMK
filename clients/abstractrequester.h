#ifndef ABSTRACTREQUESTER_H
#define ABSTRACTREQUESTER_H

#include "cclient.h"
#include <QMutex>

class AbstractRequestConvertData;

class AbstractRequester : public CClient
{
public:
    enum Errors {Ok, ErrorInit, ErrorTimeOut};

    AbstractRequester(CMsgProvider* owner,
                      AbstractRequestConvertData *dest, int size, unsigned char command, int timeout = 2);
    virtual ~AbstractRequester();
    Errors sendRequest(const unsigned char *data = nullptr, int size = 0);

    static AbstractRequester* createSondingStartRequester(CMsgProvider* owner);
    static AbstractRequester* createAverageWindSpeedRequester(CMsgProvider* owner);
    static AbstractRequester* createAverageWindDirectionRequester(CMsgProvider* owner);
    static AbstractRequester* createRealWindSpeedRequester(CMsgProvider* owner);
    static AbstractRequester* createRealWindDirectionRequester(CMsgProvider* owner);
    static AbstractRequester* createMeteo11Requester(CMsgProvider* owner);
    static AbstractRequester* createFormalizeMeteo11Requester(CMsgProvider* owner);
    static AbstractRequester* createWeatherParam1B65Requester(CMsgProvider* owner);
    static AbstractRequester* createIzmWindRequester(CMsgProvider* owner);
    static AbstractRequester* createStationStateRequester(CMsgProvider* owner);
    static AbstractRequester* createAverageWindHRequester(CMsgProvider* owner);
    static AbstractRequester* createRealWindHRequester(CMsgProvider* owner);
    static AbstractRequester* createMeteo11UpdatedRequester(CMsgProvider* owner);
    static AbstractRequester* createFormalizeMeteo11UpdatedRequester(CMsgProvider* owner);

private:
    int sendRequest(const unsigned char command);
    AbstractRequestConvertData *d_dest;
    int d_size;
    unsigned char d_command;
    int d_timeout;
    QMutex m_reqLocker;
};

#endif // ABSTRACTREQUESTER_H
