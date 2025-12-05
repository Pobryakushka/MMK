#include "abstractrequester.h"
#include "store/store.h"
#include "transfertypes.h"
#include <QReadLocker>
#include <QMutex>
#include <QMutexLocker>

static QMutex g_requestMutex;

extern Store dataStore;

AbstractRequester::AbstractRequester(CMsgProvider* owner,
    AbstractRequestConvertData* dest, int size, unsigned char command, int timeout) : CClient(owner) {
    d_dest = dest;
    d_size = size;
    d_command = command;
    d_timeout = timeout;
}

AbstractRequester::~AbstractRequester() {

}

AbstractRequester::Errors AbstractRequester::sendRequest(const unsigned char *data, int size) {
    QMutexLocker locker(&g_requestMutex);

    if (nullptr != current) {
        provider->removeMsgLink(current);
        delete current;
    }
    current = provider->createMsgObject(d_command);
    if (nullptr == current) {
        std::cout<<"Command " << (int)d_command << " sendRequest ERROR : MsgObject is NULL"<<std::endl;
        return ErrorInit;
    }
    if (d_timeout > 0)
        current->setTimeout(d_timeout);
    if (data && size)
        current->setRequestData(data, size);
    provider->put(current);
    if (0 != current->waitMessageAnswer()) {
        std::cout << "provider - " << provider << std::endl;
        provider->removeMsgLink(current);
        delete current;
        current = nullptr;
        return ErrorTimeOut;
    }
//    memcpy(d_dest, current->getAnswerData(), d_size);
    d_dest->setData(current->getAnswerData(), d_size);
    delete current;
    current = nullptr;
    return Ok;
}

AbstractRequester* AbstractRequester::createSondingStartRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.sondingStart, 1, COMMAND_1V16_SONDING_START);
}

AbstractRequester* AbstractRequester::createAverageWindSpeedRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.averageWind.speed, dataStore.requestData.averageWind.dataSize(), COMMAND_1V16_SPEED_AVERAGE);
}

AbstractRequester* AbstractRequester::createAverageWindDirectionRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.averageWind.direction, dataStore.requestData.averageWind.dataSize(), COMMAND_1V16_DIRECT_AVERAGE);
}

AbstractRequester* AbstractRequester::createRealWindSpeedRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.realWind.speed, dataStore.requestData.realWind.dataSize(), COMMAND_1V16_SPEED);
}

AbstractRequester* AbstractRequester::createRealWindDirectionRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.realWind.direction, dataStore.requestData.realWind.dataSize(), COMMAND_1V16_DIRECT);
}

AbstractRequester* AbstractRequester::createMeteo11Requester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.meteo11, Meteo11::LENGTH, COMMAND_1V16_METEO_ACCURATE);
}

AbstractRequester* AbstractRequester::createFormalizeMeteo11Requester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.formalizeMeteo11, FormalizeMeteo11::SIZEOF, COMMAND_1V16_METEO_FORMALIZE);
}

AbstractRequester* AbstractRequester::createWeatherParam1B65Requester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.weatherParameters1B65, WeatherParameters1B65::SIZEOF, COMMAND_1V16_WEATHER_PARAMETERS_1B65);
}

AbstractRequester* AbstractRequester::createIzmWindRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.izmWind, IzmWindData::SIZEOF, COMMAND_1V16_ACTUAL_WIND, 10);
}

AbstractRequester* AbstractRequester::createStationStateRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.stationState, StationState::SIZEOF, COMMAND_1V16_STATION_STATE);
}

AbstractRequester* AbstractRequester::createAverageWindHRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.averageWindHeight, WindHData::SIZEOF, COMMAND_1V16_AVERAGEWIND_HEIGHT, 5);
}

AbstractRequester* AbstractRequester::createRealWindHRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.realWindHeight, WindHData::SIZEOF, COMMAND_1V16_REALWIND_HEIGHT, 5);
}

AbstractRequester* AbstractRequester::createMeteo11UpdatedRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.meteo11upd, Meteo11Updated::SIZEOF, COMMAND_1V16_METEO11_UPDATED);
}

AbstractRequester* AbstractRequester::createFormalizeMeteo11UpdatedRequester(CMsgProvider *owner) {
    return new AbstractRequester(owner, &dataStore.requestData.formalizeMeteo11Upd, FormalizeMeteo11Updated::SIZEOF, COMMAND_1V16_FORM_METEO11_UPDATED);
}

