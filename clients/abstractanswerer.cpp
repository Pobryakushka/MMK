#include "abstractanswerer.h"
#include <thread>
#include <chrono>
#include "store/store.h"

extern Store dataStore;

AbstractAnswerer::AbstractAnswerer(CMsgProvider* owner,
    const AbstractAnswerConvertedData* source, int size, const std::string &answererName, int nanoSleep) : CClient(owner) {
    this->source = source;
    this->size = size;
    this->nanoSleep = nanoSleep;
    d_answererName = answererName;
    m_thread = std::thread(&AbstractAnswerer::threadAction, this);
    std::cout << d_answererName << " - start thread (+)" << std::endl;
}

AbstractAnswerer::~AbstractAnswerer() {
    endWork = true;
    if (m_thread.joinable())
        m_thread.join();
    std::cout << d_answererName << " - stop thread (-)" << std::endl;
}

void AbstractAnswerer::threadAction() {
    while (!endWork) {
        if (nullptr == current) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSleep));
            continue;
        }

        current->setAnswerData(source->data(), size);
//        current->setAnswerData(source->convertedData(), size);
        provider->put(current);
        current = nullptr;
        finishDataProcessing();
    }
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
