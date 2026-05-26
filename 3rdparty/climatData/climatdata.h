#ifndef CLIMATDATA_H
#define CLIMATDATA_H

#include <string>
#include <map>
#include <iostream>

class ClimatDataPrivate;

using namespace std;

class ClimatData
{
public:
//    enum HeightLevel32 {L32H00100, L32H00150, L32H00200, L32H00300, L32H00400, L32H00500, L32H00600, L32H00700, L32H00800, L32H00900, L32H01000,
//    L32H01200, L32H01400, L32H01600, L32H01800, L32H02000, L32H02200, L32H02400, L32H02600, L32H02800, L32H03000, L32H04000,
//    L32H05000, L32H06000, L32H08000, L32H10000, L32H12000, L32H14000, L32H18000, L32H22000, L32H26000, L32H30000};

//    // Объявляем набор поименованных констант для уровней высоты (L21H00100 = 0, L21H00200 = 1, ..., L21H30000 = 21)
//    enum HeightLevel21 {L21H00100, L21H00200, L21H00400, L21H00600, L21H00800, L21H01200, L21H01600, L21H02000, L21H02400, L21H02600, L21H03000,
//    L21H04000, L21H05000, L21H06000, L21H08000, L21H10000, L21H12000, L21H14000, L21H18000, L21H22000, L21H26000, L21H30000};

    struct ClimaticZone {                     // Структура "Климатическая зона" содержит следующие данные, полученные на основе исходных (дата, Ш, Д метеокомплекса):
        enum HalfYearType {COLD = 4, WARM = 5, UNDEFINED = 0};       // Объявляем набор поименованных констант для холодного (COLD = 0) и теплого (WARM = 1) полугодий
        HalfYearType halfYear;                // 1) тип полугодия,
        int          numClZone;               // 2) номер климатической зоны,
        float        latitudeClZone;          // 3) географическую широту,
        float        longitudeClZone;         // 4) географическую долготу,
        int month;

        ClimaticZone();

        // Определение типа полугодия
        void detHalfYearType (int selMonth);
        const string& zoneName() const;
    };

    struct StationInfo { //StnIndex
        float index, latitude, longitude, height, coldZone, warmZone;
    };

    struct ZoneData { //coldClZoneCoord, warmClZoneCoord
        int latFrom, latTo, longFrom, longTo, zone;
    private:
        int unused;
    };

    struct MapNotation {
        int degree, minute, second;
        // Перевод значений координат из географической записи (градусы, минуты, секунды) в десячитную запись
        static MapNotation convertFromDecimalToMapNotation(float decimalValueCoordinate) {
            MapNotation mn;
            mn.degree = int(decimalValueCoordinate);
            mn.minute = int((decimalValueCoordinate - mn.degree)*60.0);
            mn.second = int((decimalValueCoordinate - mn.degree - mn.minute/60.0)*3600.0);
            return mn;
        }

        // Перевод значений координат из десячитной записи в географическую запись (градусы, минуты, секунды)
        static float convertFromMapToDecimalNotation (float valueDegree, float valueMinute, float valueSecond) {
            return valueDegree + valueMinute/60 + valueSecond/3600;
        }
        float convertFromMapToDecimalNotation() {
            return degree + minute/60 + second/3600;
        }
    };

    struct ZMData {
        float z, m;
    };

    struct Pr{                                // Структура "Среднеклиматические значения" среднего ветра для стандартного слоя для
        float VZ;                            // 1) зональной составляющей,
        float VM;                            // 2) меридиональной составляющей
    };

    struct Sigma{                             // Структура "Среднеквадратические отклонения" среднего ветра для стандартного слоя для
        float SiVZ;                          // 1) зональной составляющей,
        float SiVM;                          // 2) меридиональной составляющей
    };

    struct Correl{                            // Структура "Коэффициенты линейной автокорреляции" между слоями для
        float CorrZ;                         // 1) зональной составляющей,
        float CorrM;                         // 2) меридиональной составляющей
    };

    ClimatData(const string &dataPath);
    ClimatData(float latitude, float longitude, int month, const string &dataPath);
    ~ClimatData();
    void setClZone(float latitude, float longitude, int month);
    const ClimaticZone& getCurrentClZone() const;
    static float distanceBetweenTwoStations (float latitudeStation1, float longitudeStation1, float latitudeStation2, float longitudeStation2);
    float rangeToCloserStation() const;
    const string& dataFileName() const;
    int stationOrderNumber() const;
    bool isDataReady() const;
    const map <int, map <int, ZMData> >& mapCorrel() const;
    static const StationInfo &stationInfo(unsigned int stationOrderNumber);
    static unsigned int stationsCount();
    static const ZoneData* coldZone(int zoneAreaNumber, unsigned int &dataCount); //zoneAreaNumber >= 1, dataCount возвращает кол-во ZoneData >=1
    static const ZoneData* warmZone(int zoneAreaNumber, unsigned int &dataCount); //для текущей зоны, или ноль если такой зоны нет
    static unsigned int coldZoneCount();
    static unsigned int warmZoneCount();

public:
    static const unsigned int MID_LAYERS_COUNT/* = 22*/;
//    static const int REAL_LAYERS_COUNT = 33;
//    const int mHLevel32[REAL_LAYERS_COUNT] = {0, 100, 150, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800, 3000, 4000, 5000, 6000, 8000, 10000, 12000, 14000, 18000, 22000, 26000, 30000};
    static const int mHLevel21[/*MID_LAYERS_COUNT*/];

    inline static unsigned int layersCount() {
        return MID_LAYERS_COUNT;
    }
    inline const int* layers() const {
        return mHLevel21;
    }
    bool hasLayer(float layer) const;
    const Sigma& sigma(float layer1, float layer2 = -1) const;
    const Pr& pr(float layer1, float layer2 = -1) const;
    const Correl& correl(float layer1, float layer2) const;
private:
    void checkLayers(float &layer1, float &layer2) const;
    static const ZoneData * zoneData(int zoneAreaNumber, unsigned int &dataCount, bool coldZones);
    ClimatDataPrivate *d;
};



#endif // CLIMATDATA_H
