#include "climatdata.h"
#include <cmath>
//#include <iostream>

#include "climatdata_global.h"
#include "climatdataprivate.h"
#include <cassert>

using namespace GlobalClimatData;

const unsigned int ClimatData::MID_LAYERS_COUNT = 22;
const int ClimatData::mHLevel21[ClimatData::MID_LAYERS_COUNT] = {100, 200, 400, 600, 800, 1200, 1600, 2000, 2400, 2600, 3000, 4000, 5000, 6000, 8000, 10000, 12000, 14000, 18000, 22000, 26000, 30000};


ClimatData::ClimatData(const string &dataPath) : d(new ClimatDataPrivate(dataPath))          //TODO: Ќадо сделать значени€ по умолчанию!!!!!!!!!!
{

}

ClimatData::ClimatData(float latitude, float longitude, int month, const string &dataPath) :
    d(new ClimatDataPrivate(latitude, longitude, month, dataPath))
{

}

ClimatData::~ClimatData()
{
    delete d;
}

void ClimatData::setClZone(float latitude, float longitude, int month)
{
    d->setClZone(latitude, longitude, month);
}



//---------------------------------------------------------------------------------------------------------------------------------------------------



// ќпределение рассто€ни€ между двум€ станци€ми
float ClimatData::distanceBetweenTwoStations (float latitudeStation1, float longitudeStation1, float latitudeStation2, float longitudeStation2) {
    // latitudeStation1, longitudeStation1  - географическа€ широта и догота станции 1 в градусах,
    // latitudeStation2, longitudeStation2  - географическа€ широта и догота станции 2 в градусах
    return acos(sin(latitudeStation1*Pi/180)*sin(latitudeStation2*Pi/180) + cos(latitudeStation1*Pi/180)*cos(latitudeStation2*Pi/180)*
                cos((longitudeStation1 - longitudeStation2)*Pi/180))*EarthRadius;
}

float ClimatData::rangeToCloserStation() const {
    return d->Lmin;
}

const string &ClimatData::dataFileName() const {
    //        std::cout << nameDataFile << endl;
    return d->nameDataFile;
}

int ClimatData::stationOrderNumber() const {
    return d->iStation;
}

bool ClimatData::isDataReady() const {
    return d->dataReady;
}

const map<int, map<int, ClimatData::ZMData> > &ClimatData::mapCorrel() const {
    return d->m_mapCorrel;
}

const ClimatData::StationInfo &ClimatData::stationInfo(unsigned int stationOrderNumber)
{
    if (stationOrderNumber > 0 && stationOrderNumber < STATIONS_COUNT)
        return reinterpret_cast<const StationInfo&>(*StnIndex[stationOrderNumber]);
    else
        return reinterpret_cast<const StationInfo&>(*StnIndex[0]);
}

unsigned int ClimatData::stationsCount()
{
    return STATIONS_COUNT;
}

const ClimatData::ZoneData * ClimatData::zoneData(int zoneAreaNumber, unsigned int &dataCount, bool coldZones)
{
    const ZoneData *retVal = NULL;
    const ZoneData *zone = coldZones ? reinterpret_cast<const ZoneData*>(coldClZoneCoord) : reinterpret_cast<const ZoneData*>(warmClZoneCoord);
    dataCount = 0;
    int zonesCount = coldZones ? ColdZoneDataCount : WarmZoneDataCount;
    for (int i = 0; i < zonesCount; i++) {
        if (zone->zone == zoneAreaNumber) {
            if (dataCount == 0)
                retVal = zone;
            dataCount++;
        }
        else if (dataCount > 0)
            break;
        zone++;
    }
    return retVal;
}

const ClimatData::ZoneData *ClimatData::coldZone(int zoneAreaNumber, unsigned int &dataCount)
{
    return zoneData(zoneAreaNumber, dataCount, true);
}

const ClimatData::ZoneData *ClimatData::warmZone(int zoneAreaNumber, unsigned int &dataCount)
{
    return zoneData(zoneAreaNumber, dataCount, false);
}

unsigned int ClimatData::coldZoneCount()
{
    return ColdClZoneAreaCount;
}

unsigned int ClimatData::warmZoneCount()
{
    return WarmClZoneAreaCount;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------



const ClimatData::ClimaticZone &ClimatData::getCurrentClZone() const {
    return d->ClZone;
}

void ClimatData::checkLayers(float &layer1, float &layer2) const {
    if (layer2 == -1)
        layer2 = mHLevel21[0];
    if (layer1 < layer2) {
        float tmp = layer2;
        layer2 = layer1;
        layer1 = tmp;
    }
}

bool ClimatData::hasLayer(float layer) const {
    for (unsigned int i = 0; i < MID_LAYERS_COUNT; i++) {
        if (layer == mHLevel21[i])
            return true;
    }
    return false;
}

const ClimatData::Sigma &ClimatData::sigma(float layer1, float layer2) const {
    checkLayers(layer1, layer2);
    return reinterpret_cast<const Sigma&>(d->mapSigma.at(static_cast<int>(layer1)).at(static_cast<int>(layer2)));
}

const ClimatData::Pr &ClimatData::pr(float layer1, float layer2) const {
    checkLayers(layer1, layer2);
    return reinterpret_cast<const Pr&>(d->mapPr.at(static_cast<int>(layer1)).at(static_cast<int>(layer2)));
}

const ClimatData::Correl &ClimatData::correl(float layer1, float layer2) const {
    checkLayers(layer1, layer2);
    return reinterpret_cast<const Correl&>(d->m_mapCorrel.at(static_cast<int>(layer1)).at(static_cast<int>(layer2)));
}

//---------------------------ClimaticZone-------------------------------------------


ClimatData::ClimaticZone::ClimaticZone() : halfYear(UNDEFINED), numClZone(0) {
}

void ClimatData::ClimaticZone::detHalfYearType(int selMonth) {
    if (selMonth <= 0 || selMonth > 12) halfYear = UNDEFINED;
    else if (selMonth <= 3 || selMonth >= 10) halfYear = COLD;
    else if (selMonth >= 4 && selMonth <=  9) halfYear = WARM;
}

const string &ClimatData::ClimaticZone::zoneName() const
{
    switch (halfYear) {
    case COLD:
        assert(numClZone > 0 || numClZone > ColdClZoneAreaCount);
        return coldClZoneArea[numClZone-1];
        break;
    case WARM:
        assert(numClZone > 0 || numClZone > WarmClZoneAreaCount);
        return warmClZoneArea[numClZone-1];
        break;
    case UNDEFINED:
    default:
        return coldClZoneArea[0];
        break;
    }
}
