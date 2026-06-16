#ifndef ABSTRACTANSWERER_H
#define ABSTRACTANSWERER_H

#include <cclient.h>
#include <thread>
#include <atomic>

class AbstractAnswerConvertedData;

class AbstractAnswerer : public CClient {
    int nanoSleep;
    const AbstractAnswerConvertedData* source;
    int size;
    std::thread m_thread;
    std::atomic<bool> endWork{false};
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
    void threadAction();

};

#endif
