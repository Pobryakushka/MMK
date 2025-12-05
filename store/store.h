#ifndef STORE_H
#define STORE_H

#include "storetypes.h"
#include <QDateTime>

class Store {

public:
    struct AnswerData {
        ComplexCoordinates latitude, longitude;
        ComplexPosition position;
        Meteo11 meteo11;
        DateTime dt;
        DateTimeAmsT dtAmsT;
    } answerData;

    struct RequestData {
        RequestData() : averageWind(WindData::AVERAGE_WIND_LENGTH), realWind(WindData::REAL_WIND_LENGTH) {}
        SondingStartAnswer sondingStart;
        WindData averageWind, realWind;
        Meteo11 meteo11;
        FormalizeMeteo11 formalizeMeteo11;
        WeatherParameters1B65 weatherParameters1B65;
        IzmWindData izmWind;
        StationState stationState;
        WindHData realWindHeight, averageWindHeight;
        Meteo11Updated meteo11upd;
        FormalizeMeteo11Updated formalizeMeteo11Upd;
    } requestData;


public:
    Store();
    ~Store();
    static void swapBytes(char* bytes, int size);

};

#endif
