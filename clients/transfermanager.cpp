#include "transfermanager.h"
#include <iostream>
#include <list>
#include <QSerialPort>
#include "abstractanswerer.h"
#include "abstractrequester.h"


TransferManager::TransferManager() : comManager1V16(nullptr), msgProvider1V16(nullptr), cmdsFrom1V16(false) {
    isStarted = false;
    inited = false;
}

TransferManager::~TransferManager() {
    stop();
}

void TransferManager::del() {
//    stop();
    if (nullptr != comManager1V16) {
//        std::cout << "4" << std::endl;
        std::list<CComand*>* list = comManager1V16->getCommandList();
//        std::cout << "5" << std::endl;
        std::list<CComand*>::const_iterator it;
//        std::cout << "6" << std::endl;
        for (it = list->begin(); it != list->end(); it++) {
            CComand *c = (CComand*)*it;
//            std::cout << "7 " << c << std::endl;
            if (c != nullptr && c->isIncoming()) {
                AbstractAnswerer* a = TransferManager::answerer(c->commandAttr->id);
//                std::cout << "8 " << a << std::endl;
                if (a != nullptr) {
//                    std::cout << "9" << std::endl;
                    delete a;
                }
                delete c;
//                std::cout << "10" << std::endl;
            }
        }
        commands1V16.clear();
        list->clear();
        delete comManager1V16;
        comManager1V16 = nullptr;
    }
    else
        return;
    if (nullptr != msgProvider1V16) {
        delete msgProvider1V16;
        msgProvider1V16 = nullptr;
    }
    for (unsigned int i = 0; i < commands1V16.size(); i++) {
        delete commands1V16[i];
    }
    inited = false;
}

void TransferManager::init(CComPort *comport) {
    del();

    comManager1V16 = new CPortManager(comport);

    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.direction)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.directionAverage)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.averageHeight)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.realHeight)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.latitude)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.longitude)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.meteo)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.meteoAccurate)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.meteoFormalize)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.parameters1b65)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.position)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.speed)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.speedAverage)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.actualWind)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.dateTime)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.weatherFrom1v16)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.stationState)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.sondingStart)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.meteo11Upd)));
    commands1V16.push_back(new CComand(comManager1V16, &(cmdsFrom1V16.formMeteoUpd)));

    for (unsigned int i = 0; i < commands1V16.size(); i++) {
        comManager1V16->addCommand(commands1V16[i]);
    }

    msgProvider1V16 = new CMsgProvider(comManager1V16, commands1V16.size());

    inited = true;

    initClients();

    start();
}

bool TransferManager::start() {
    if (isStarted) return true;
    if (!inited) return false;
    if (comManager1V16->port->isOpen()) {
        comManager1V16->startThread();
        msgProvider1V16->startThread();
    }
    else
        return false;

    isStarted = true;
    return true;
}

void TransferManager::stop() {
    if (!isStarted) return;

    isStarted = false;
    this->comManager1V16->stopThread();
    this->msgProvider1V16->stopThread();
    del();
}

void TransferManager::initClients() {
    //answerer
    msgProvider1V16->addAssociate(cmdsFrom1V16.latitude.id, AbstractAnswerer::createLatitudeAnswerer(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.longitude.id, AbstractAnswerer::createLongitudeAnswerer(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.dateTime.id, AbstractAnswerer::createDateTimeAnswerer(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.meteo.id, AbstractAnswerer::createMeteo11Answerer(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.position.id, AbstractAnswerer::createComplexPositionAnswerer(msgProvider1V16));
    //requester
    msgProvider1V16->addAssociate(cmdsFrom1V16.speed.id, AbstractRequester::createRealWindSpeedRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.direction.id, AbstractRequester::createRealWindDirectionRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.speedAverage.id, AbstractRequester::createAverageWindSpeedRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.directionAverage.id, AbstractRequester::createAverageWindDirectionRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.meteoAccurate.id, AbstractRequester::createMeteo11Requester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.meteoFormalize.id, AbstractRequester::createFormalizeMeteo11Requester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.parameters1b65.id, AbstractRequester::createWeatherParam1B65Requester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.actualWind.id, AbstractRequester::createIzmWindRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.stationState.id, AbstractRequester::createStationStateRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.sondingStart.id, AbstractRequester::createSondingStartRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.averageHeight.id, AbstractRequester::createAverageWindHRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.realHeight.id, AbstractRequester::createRealWindHRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.meteo11Upd.id, AbstractRequester::createMeteo11UpdatedRequester(msgProvider1V16));
    msgProvider1V16->addAssociate(cmdsFrom1V16.formMeteoUpd.id, AbstractRequester::createFormalizeMeteo11UpdatedRequester(msgProvider1V16));

}

AbstractAnswerer* TransferManager::answerer(unsigned char command) {
    return dynamic_cast<AbstractAnswerer*>(msgProvider1V16->client(command));
}

AbstractRequester* TransferManager::requester(unsigned char command) {
    return dynamic_cast<AbstractRequester*>(msgProvider1V16->client(command));
}
