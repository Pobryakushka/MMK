#include "abstractanswerer.h"
#include <timespec.h>
#include "store/store.h"

extern Store dataStore;

AbstractAnswerer::AbstractAnswerer(CMsgProvider* owner,
    const AbstractAnswerConvertedData* source, int size, const std::string &answererName, int nanoSleep) : CClient(owner) {
    this->source = source;
    this->size = size;
    this->nanoSleep = nanoSleep;
    d_answererName = answererName;
    this->endWork = false;
    pthread_create(&idthread, nullptr, threadActionCheckData, (void*)this);
    std::cout << d_answererName << " - start thread (+)" << std::endl;
}

AbstractAnswerer::~AbstractAnswerer() {
    endWork = true;
    pthread_join(idthread, nullptr);
    std::cout << d_answererName << " - stop thread (-)" << std::endl;
}

void* AbstractAnswerer::threadActionCheckData(void* argObject) {
    if (nullptr == argObject)
        return nullptr;
    AbstractAnswerer* sender = (AbstractAnswerer*)argObject;
    struct timespec inTime;
    inTime.tv_sec = 0;
    inTime.tv_nsec = sender->nanoSleep;
    while ((nullptr != sender) && (true != sender->endWork)) {
        if (nullptr == sender->current) {
            nanosleep(&inTime, nullptr);
            continue;
        }

        sender->current->setAnswerData(sender->source->data(), sender->size);
//        sender->current->setAnswerData(sender->source->convertedData(), sender->size);
        sender->provider->put(sender->current);
        sender->current = nullptr;
        sender->finishDataProcessing();
    }
    return nullptr;
}

AbstractAnswerer* AbstractAnswerer::createLatitudeAnswerer(CMsgProvider *owner) {
    return new AbstractAnswerer(owner, &dataStore.answerData.latitude, ComplexCoordinates::SIZEOF, "latitude answerer");
}

AbstractAnswerer* AbstractAnswerer::createLongitudeAnswerer(CMsgProvider *owner) {
    return new AbstractAnswerer(owner, &dataStore.answerData.longitude, ComplexCoordinates::SIZEOF, "longitude answerer");
}

AbstractAnswerer* AbstractAnswerer::createComplexPositionAnswerer(CMsgProvider *owner) {
    return new AbstractAnswerer(owner, &dataStore.answerData.position, ComplexPosition::SIZEOF, "position answerer");
}

AbstractAnswerer* AbstractAnswerer::createMeteo11Answerer(CMsgProvider *owner) {
    return new AbstractAnswerer(owner, &dataStore.answerData.meteo11, Meteo11::LENGTH, "meteo11 answerer");
}

//AbstractAnswerer* AbstractAnswerer::createDateTimeAnswerer(CMsgProvider *owner) {
//    return new AbstractAnswerer(owner, &dataStore.answerData.dt, DateTime::SIZEOF, "datetime answerer");
//}
AbstractAnswerer* AbstractAnswerer::createDateTimeAnswerer(CMsgProvider *owner) {
    return new AbstractAnswerer(owner, &dataStore.answerData.dtAmsT, DateTimeAmsT::SIZEOF, "datetime answerer");
}

//---

//AbstractAnswerer* AbstractAnswerer::createArbitraryHeightAnswerer(CMsgProvider *owner) {
//    return new AbstractAnswerer(owner, &dataStore.answerData.arbitraryHeight, ArbitaryHeight::SIZEOF, "arbitraryHeigh answerer");}

//AbstractAnswerer *AbstractAnswerer::createWeatherAnswerer(CMsgProvider *owner) {
//    return new AbstractAnswerer(owner, &dataStore.answerData.weather, WeatherParameters1B65::SIZEOF, "weather answerer");
//}

//---
