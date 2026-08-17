#ifndef CLIMATDATAPRIVATE_H
#define CLIMATDATAPRIVATE_H

#include "climatdata.h"

class ClimatDataPrivate {
public:
    // Объявляем набор поименованных констант для использования при вызове функции getRequiredParameter
    enum pParameter {pHeight, pTotalYears, pYearsWithData, pNumProcessedLaunchs};
    // Объявляем набор поименованных констант для использования при вызове функции getDataFromString
    enum readingDataType {LINE = 1, BLOCK};

    struct StationInformation {               // Структура "Сведения о станции" содержит следующие данные, полученные из файла справочника атмофсферы:
        int   synopticIndexStation;           // 1) cиноптический индекс станции,
        float latitudeStation;                // 2) широта станции,
        float longitudeStation;               // 3) долгота станции,
        float orthometricHeightStation;       // 4) высота станции над поверхностью земли,
        int   numClZoneStation;               // 5) номер климатической зоны станции,
        int   numProcessedLaunchsStation;     // 6) количество проанализированных выпусков радиозондов с данной станции.
    };
    struct StationMeteoParameters {           // Структура "Метеопараметры станции" содержит следующие данные, полученные из файла справочника атмофсферы:
//        string P;                            // 1) среднеклиматическое атмосферное давление,
//        string Tv;                           // 2) среднеклиматическая виртуальная температура,
//        string Vx;                           // 3) среднеклиматическая зональная составляющая действительного ветра,
//        string Vy;                           // 4) среднеклиматическая меридиональная составляющая действительного ветра,
//        string sTv;                          // 5) среднеклиматическая среднеслойная виртуальная температура,
        string sVx;                          // 6) среднеклиматическая среднеслойная зональная составляющая действительного ветра,
        string sVy;                          // 7) среднеклиматическая среднеслойная меридиональная составляющая действительного ветра.
    };


    ClimatDataPrivate(const string &dataPath);
    ClimatDataPrivate(float latitude, float longitude, int month, const string &dataPath);
    void setClZone(float latitude, float longitude, int selMonth);
private: // не проверенные методы, в расчете не учавствуют
    string getRequiredParameter(const string &dataRequiredStation, pParameter pParametr) const;
    string getDataFromString (string dataString, char delimiter, int numArg, ...);
private:
    int parseMeteoParameters();
    int readDataFromFile(string pathDataFile);
    int getStation();
    string getNameDataFile ();
    void detHalfYearType (int selMonth);
    void readMeteoParam(const string &dataString, map<int, map<int, ClimatData::ZMData> > &m, int p, bool zValue);
    void checkLayers(float &layer1, float &layer2) const;

    ClimatData::ClimaticZone           ClZone;
//    StationInformation     StationInfo;

    int iStation;
    float Lmin;
    string dataPath;
    bool dataReady;
    map <int, map <int, ClimatData::ZMData> > m_mapCorrel;
    map <int, map <int, ClimatData::ZMData> > mapSigma;
    map <int, map <int, ClimatData::ZMData> > mapPr;
    StationMeteoParameters StationMeteoParam;
    string   nameDataFile;
    friend class ClimatData;
};

#endif // CLIMATDATAPRIVATE_H
