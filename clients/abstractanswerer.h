#ifndef ABSTRACTANSWERER_H
#define ABSTRACTANSWERER_H

#include <cclient.h>
#include <pthread.h>

class AbstractAnswerConvertedData;

class AbstractAnswerer : public CClient {
    int nanoSleep;
    const AbstractAnswerConvertedData* source;
    int size;
    pthread_t idthread;
    bool endWork;
    std::string d_answererName;

public:
    AbstractAnswerer(CMsgProvider* owner, const AbstractAnswerConvertedData* source,
        int size, const string &answererName = "abstract answerer", int nanoSleep = 200000000);
    virtual ~AbstractAnswerer();

    static AbstractAnswerer* createLatitudeAnswerer(CMsgProvider* owner);
    static AbstractAnswerer* createLongitudeAnswerer(CMsgProvider* owner);
    static AbstractAnswerer* createComplexPositionAnswerer(CMsgProvider *owner);
    static AbstractAnswerer* createMeteo11Answerer(CMsgProvider* owner);
    static AbstractAnswerer* createDateTimeAnswerer(CMsgProvider* owner);
    static AbstractAnswerer* createArbitraryHeightAnswerer(CMsgProvider* owner);
    static AbstractAnswerer* createWeatherAnswerer(CMsgProvider* owner);

protected:
    static void* threadActionCheckData(void*);
    pthread_t getThreadID() { return idthread; }

};

#endif
