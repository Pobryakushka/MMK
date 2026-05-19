#include "climatdataprivate.h"
#include "climatdata_global.h"
#include <stdarg.h>
#include <fstream>
#include <iostream>

using namespace GlobalClimatData;

ClimatDataPrivate::ClimatDataPrivate(const string &dataPath) : dataReady(false)
{
    this->dataPath = dataPath;
}

ClimatDataPrivate::ClimatDataPrivate(float latitude, float longitude, int month, const string &dataPath) : dataReady(false)
{
    this->dataPath = dataPath;
    setClZone(latitude, longitude, month);
}

void ClimatDataPrivate::setClZone(float latitude, float longitude, int selMonth) {
    dataReady = false;
    ClZone.detHalfYearType(selMonth);
    ClZone.latitudeClZone = latitude;             // географическая широта
    ClZone.longitudeClZone = longitude;           // географическая долгота
    ClZone.month = selMonth;

    ClZone.numClZone = 0;

    int i;
    if (ClZone.halfYear == ClimatData::ClimaticZone::COLD) {
        for (i = 0; i <= 19; i++) {
            if (ClZone.latitudeClZone  >= coldClZoneCoord[i][0][0] && ClZone.latitudeClZone  <= coldClZoneCoord[i][0][1] &&
                    ClZone.longitudeClZone >= coldClZoneCoord[i][1][0] && ClZone.longitudeClZone <= coldClZoneCoord[i][1][1]) {
                ClZone.numClZone = coldClZoneCoord[i][2][0];
            }
        }
    }
    else if (ClZone.halfYear == ClimatData::ClimaticZone::WARM) {
        for (i = 0; i <= 19; i++) {
            if (ClZone.latitudeClZone  >= warmClZoneCoord[i][0][0] && ClZone.latitudeClZone  <= warmClZoneCoord[i][0][1] &&
                    ClZone.longitudeClZone >= warmClZoneCoord[i][1][0] && ClZone.longitudeClZone <= warmClZoneCoord[i][1][1]) {
                ClZone.numClZone = warmClZoneCoord[i][2][0];
            }
        }
    }
    std::cout << getStation() << std::endl;
    if (getStation()) {
//        std::cout << readDataFromFile(dataPath) << std::endl;
        if (readDataFromFile(dataPath)) {
            parseMeteoParameters();
            dataReady = true;
        }
        StationMeteoParam.sVx.clear();
        StationMeteoParam.sVy.clear();
    }
}

// Определение подходящей ближайшей станции
int ClimatDataPrivate::getStation() {
    iStation = 0;
    Lmin = EarthRadius/2;

    int iHalfYear;
    if (ClZone.halfYear ==  ClimatData::ClimaticZone::WARM) iHalfYear = 5;                 // в зависимости от определенного полугодия
    else if (ClZone.halfYear ==  ClimatData::ClimaticZone::COLD) iHalfYear = 4;                 // Определяем столбец массива, из которого следует брать номер зоны станции
    else return 0;

    double L;
    if (ClZone.numClZone != 0) {
        for (int i = 0; i <= 279; i++){
            if (StnIndex[i][iHalfYear] == ClZone.numClZone &&                           // Проверяем, находятся ли эта станция в нашей зоне и
                    StnIndex[i][5 + ClZone.month] == ClZone.month){                     // имеются ли для i-ой в массиве метеостанции данные за выбранный месяц
                L =  ClimatData::distanceBetweenTwoStations(ClZone.latitudeClZone, ClZone.longitudeClZone, StnIndex[i][1], StnIndex[i][2]); // Определяем расстояние от точки с заданными координатами до i-ой станции
                if (L <= Lmin) {                                               // Проверяем, находится ли i-ая станция ближе к точке с заданными координатами и
                    Lmin = L;
                    iStation = i;
                }
            }
        }
        nameDataFile = getNameDataFile();
        return iStation;
    }
    else
        return 0;
}

//Чтение требуемых параметров из строки dataRequiredStation, содержащей всю имеющуюся информацию по станции из справочника
//Не проверено, скорее всего работает не верно. В рачете не учавствует
string ClimatDataPrivate::getRequiredParameter(const string &dataRequiredStation, pParameter Parameter) const{
    unsigned int i, startPosition, endPosition, lengthValue;
    int iTab = 0;                                                 // Счетчик символа табуляции "\t"

    string valueParameter = "";

    switch (Parameter){
       case pHeight:                                              // Считываем высоту станции над уровнем моря

       for (i = 1; i <= dataRequiredStation.find("\n"); i++){
            if (dataRequiredStation.at(i-1) == '\t'){
                iTab++;
                if (iTab == 4) startPosition = i;
                if (iTab == 5) endPosition = i;
            }
        }
        lengthValue = endPosition - startPosition - 1;
        valueParameter = dataRequiredStation.at(startPosition);
        for (i = 1; i <= lengthValue - 1; i++) valueParameter = valueParameter + dataRequiredStation.at(startPosition + i);
        break;

        case pTotalYears:                                         // Считываем кол-во проанализированных лет
//        qDebug() << "\n" << getDataFromString(dataRequiredStation, "\t", BLOCK, 9, 10);
        break;

        case pYearsWithData:                                      // Считываем кол-во лет с имеющимися данными
//        qDebug() << "\n" << getDataFromString(dataRequiredStation, "\n", BLOCK, 2, 2);
//        qDebug() << "\n" << getDataFromString(dataRequiredStation, "\n", LINE, 2);
        break;

        case pNumProcessedLaunchs:                                // Считываем кол-во обработанных выпусков
        break;
    }

    return valueParameter;
}

// Считывает данные из строки, имеющей заданные символы-разделители
//Не проверено, скорее всего работает не верно. В рачете не учавствует
string ClimatDataPrivate::getDataFromString (string dataString, char delimiter, int numArg, ...){

    // numArg = LINE   --> функция возвращает часть строки dataString, которая содержится между символами-разделителями delimiter с порядковыми номерами (k1 - 1) и k1, где k1 - третий аргумент фнукции
        // numArg = BLOCK  --> функция возвращает строки, которая содержится между символами-разделителями delimiter с порядковыми номерами от (k1 - 1) до k2, где k2 - четвертый аргумент функции
        //
        // getDataFromString(dataString, "\t", BLOCK, 0, 0) - выдает предупреждение о попытке вывода строки нулевой длины, заключенной между двумя символами табуляции "\t"
        // getDataFromString(dataString, "\t", BLOCK, 0, 2) - возвращает строку с первого элемента до элемента (включительно), после которого идет 2-ой символ табуляции "\t"
        // getDataFromString(dataString, "\t", BLOCK, 2, 4) - возвращает строку с элемент, перед которым идет 2-ой символ табуляции "\t" до элемента (включительно), после которого идет 4-ый символ табуляции "\t"
        //
        // getDataFromString(dataString, "\n", LINE, 2)     - возвращает строку №2, содержащую разделитель "\n"
        // getDataFromString(dataString, "\n", BLOCK, 2, 2) - возвращает строку №2, содержащую разделитель "\n"
        // getDataFromString(dataString, "\n", BLOCK, 2, 4) - возвращает строки №2, 3, 4, содержащие разделители "\n"

    va_list argList;                                             // список аргументов функции
    va_start(argList, numArg);
    int iArg = 0, *mArg = new int[numArg];
    while(iArg != numArg){
        mArg[iArg] = va_arg(argList, int);
        iArg++;
//        qDebug() << "\n" << "iArg = " << iArg << ", mArg = " << mArg[iArg - 1];
    }
    va_end(argList);

    int i = 0, iN = 0;
    int d = 1, startPosition = -1, endPosition, lengthDataLine;   // iN - Счетчик символа перевода строки "\n"
    string dataLine = "";

    if (delimiter == '\n') d = 0;

    switch (numArg){
        case LINE:
        while(iN != mArg[0]) {                                     // Определяем нужную строку, задаваясь количеством символа перевода строки "\n"
            if (dataString.at(i) == delimiter){
                iN++;
                if (iN == mArg[0] - 1) startPosition = i;
                if (iN == mArg[0]) endPosition = i + d;
            }
            i++;
        }
//        qDebug() << dataString.at(startPosition);
//        qDebug() << "\n" << dataString.at(endPosition);
        break;

        case BLOCK:
        if ((delimiter == '\t') && (mArg[0] == mArg[1])) // Проверка правильност ивызова функции. Если начало совпадает с окончанием блока с символами-разделителями "\t", то выводим следующее сообщение
            return "Попытка вывода строки нулевой длины, заключенной между двумя символами табуляции";
        while(iN != mArg[1]){                                     // Определяем нужную строку, задаваясь количеством символа перевода строки "\n"
            if (dataString.at(i) == delimiter) {
                iN++;
                if (iN == mArg[0] - 1 + d) startPosition = i;
                if (iN == mArg[1]) endPosition = i + d;
            }
            i++;
        }
        break;
    }

    startPosition = startPosition + 1;
    lengthDataLine = endPosition - startPosition - 1;
    dataLine = dataString.at(startPosition);
    for (i = 1; i <= lengthDataLine - 1; i++) dataLine = dataLine + dataString.at(startPosition + i);

    return dataLine;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

// Определение файла справочника атмосферы для открытия и чтения данных
string ClimatDataPrivate::getNameDataFile() {
    string FileName, HalfYear, Zone, Month;

    if (ClZone.month < 10)
        Month = "0" + to_string(ClZone.month);
    else
        Month = to_string(ClZone.month);

    if (ClZone.halfYear == ClimatData::ClimaticZone::COLD){
        HalfYear = "cold";
        if (StnIndex[iStation][4] < 10)
            Zone = "0" + to_string(static_cast<int>(StnIndex[iStation][4]));
        else
            Zone = to_string(static_cast<int>(StnIndex[iStation][4]));
    };

    if (ClZone.halfYear == ClimatData::ClimaticZone::WARM){
        HalfYear = "warm";
//        cout << HalfYear << endl;
        if (StnIndex[iStation][5] < 10)
            Zone = "0" + to_string(static_cast<int>(StnIndex[iStation][5]));
        else
            Zone = to_string(static_cast<int>(StnIndex[iStation][5]));
//        cout << Zone << endl;
    };

    FileName = HalfYear + Zone + Month + ".out";
    return FileName;
}

int ClimatDataPrivate::readDataFromFile(string pathDataFile) {
    if (iStation <= 0)
        return 0;
    string station_string;
    cout << pathDataFile + nameDataFile << endl;
    ifstream ifs((pathDataFile + nameDataFile).data());

    if(!ifs.is_open()){
        return 0;
    }

    ifs.seekg(0, ifs.end);
    int bytesCount = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    string dataRequiredStation;
    dataRequiredStation.resize(bytesCount);
    ifs.read(&*dataRequiredStation.begin(), bytesCount);
    ifs.close();
    station_string  = string("Станция:") + "\t" +  to_string(static_cast<int>(StnIndex[iStation][0]));
    cout << station_string << endl;

    unsigned int aPos = dataRequiredStation.find(station_string);
//    cout << aPos << "   " << bPos << endl;
    if (aPos != string::npos) {
        dataRequiredStation.erase(0, aPos);    // Отбрасываем данные станции, выше станции с нужным номером
        unsigned long bPos = dataRequiredStation.find(string("Станция:"), 2);
        if (bPos != string::npos)
            dataRequiredStation.erase(bPos, dataRequiredStation.size() - bPos);    // Отбрасываем данные станции, следующей за станцией с нужным номером
//        dataRequiredStation.shrink_to_fit();
//        cout << dataRequiredStation << endl;
//        aPos = dataRequiredStation.find("P\tМесяц:", 1);
//        bPos =  dataRequiredStation.find("Tv\tМесяц:", 1);
//        StationMeteoParam.P = dataRequiredStation.substr(aPos, bPos - aPos);             // Записываем в структуру атмосферное давление
//        aPos = bPos;
//        bPos = dataRequiredStation.find("Vx\tМесяц:", 1);
//        StationMeteoParam.Tv = dataRequiredStation.substr(aPos, bPos - aPos);            // Записываем в структуру виртуальную температуру
//        aPos = bPos;
//        bPos = dataRequiredStation.find("Vy\tМесяц:", 1);
//        StationMeteoParam.Vx = dataRequiredStation.substr(aPos, bPos - aPos);            // Записываем в структуру зональную составл. действит. ветра
//        aPos = bPos;
//        bPos = dataRequiredStation.find("Статистика в слоях", 1);
//        StationMeteoParam.Vy = dataRequiredStation.substr(aPos, bPos - aPos);            // Записываем в структуру меридиональную составл. действит. ветра
//        aPos = bPos;
        bPos = dataRequiredStation.find("sVx\tМесяц:", 1);
//        StationMeteoParam.sTv = dataRequiredStation.substr(aPos, bPos - aPos);           // Записываем в структуру среднеслойную виртуальную температуру
        aPos = bPos;
        bPos = dataRequiredStation.find("sVy\tМесяц:", 1);
        if (aPos == string::npos || bPos == string::npos)
            return 0;
        StationMeteoParam.sVx = dataRequiredStation.substr(aPos, bPos - aPos);           // Записываем в структуру среднеслойную зональную составл. действит. ветра
        aPos = bPos;
        StationMeteoParam.sVy = dataRequiredStation.substr(aPos, dataRequiredStation.size() - aPos);  // Записываем в структуру среднеслойную меридиональную составл. действит. ветра
        return 1;
    }
    else
        return 0;
}

int ClimatDataPrivate::parseMeteoParameters() {
    readMeteoParam(StationMeteoParam.sVx, mapPr, 4, true);
    readMeteoParam(StationMeteoParam.sVy, mapPr, 4, false);
    readMeteoParam(StationMeteoParam.sVx, mapSigma, 5, true);
    readMeteoParam(StationMeteoParam.sVy, mapSigma, 5, false);
    readMeteoParam(StationMeteoParam.sVx, m_mapCorrel, 8, true);
    readMeteoParam(StationMeteoParam.sVy, m_mapCorrel, 8, false);
    return 1;
}

void ClimatDataPrivate::readMeteoParam(const string &dataString, map<int, map <int, ClimatData::ZMData> > &m, int p, bool zValue) { //zValue или mValue
            // Порядковый номер параметра, который требуется считать
            // p = 1 - номер k-уровня (данный параметр не требуется выводить),
            // p = 2 - количество лет, для которых имелись выпуски с данными на n-уровне и k-уровне
            // p = 3 - количество выпусков, для которых имелись данные на n-уровне и k-уровне
            // p = 4 - среднее значение метеопараметра на n-уровне
            // p = 5 - среднеквадратичное отклонение метеопараметра на n-уровне
            // p = 6 - среднее значение метеопараметра на k-уровне
            // p = 7 - среднеквадратичное отклонение метеопараметра на k-уровне
            // p = 8 - коэффициент линейной автокорреляции для n-уровня и k-уровня
    int i, j, q = 0, mStartIndexTabP[253];                          // (1+22)/2*22 = 253
    for(i = 0; i <= 21; i++)
        for (j = 0; j <= i; j++){
            mStartIndexTabP[q] = 4*(i*i+i+1)+8*j;                   // Записываем в массив порядковый номер символа табуляции "\t" для использования при поиске значения требуемого метеопараметра
            q++;
        }

    int t = 0, iTab = 0;                                            // t  - счетчик элемента строки (каретка); iTab - счетчик символа табуляции "\t"; iN - счетчик символа перевода строки "\n"
    int qS = 0, qE = 0;                                             // qS - счетчик кол-ва начал строки с нужным значением метеопараметра; qE - счетчик кол-ва окончаний строки...
    int startPosition, endPosition, lengthDataLine;
    bool sP = false, eP = false;                                    // sP = true, если найдено начало строки после заданного порядкового символа табуляции "\t"; eP = true, если найдено окончание строки...
    string dataLine;
    double *mData = new double[254];                                // (1+22)/2*22 = 253 -> 254

    while(qE != 252 + 1){                                           // Количество символов табуляции "\t" соответствует количеству элементов массива mStartIndexTabP, увеличенному на единицу
        if (dataString.at(t) == '\t') iTab++;                       // Считаем порядковый номер найденного символа табуляции "\t"
            if (qS < 253 && iTab == mStartIndexTabP[qS] + p){
                sP = true; startPosition = t + 1;
                qS++;
            }
            if ( ( ((p != 8) && (iTab == mStartIndexTabP[qE] + p + 1)) ||                                       // Для p = 1..7 ищем символ табуляции "\t", который идет после значения требуемого параметра
                   ((p == 8) && ((iTab == mStartIndexTabP[qE] + p + 1) || (dataString.at(t) == '\n')) ) ) &&    // Для p = 8 ищем либо символ табуляции "\t", либо символ перевода строки "\n", который идет после...
                 (sP == true) ){                                                                                // Проверка нужна, чтобы не перепутать, где находится каретка (перед или после требуемого параметра)
                    eP = true; endPosition = t + 1;
                    qE++;
            }
            if ((sP == true) && (eP == true)){
                lengthDataLine = endPosition - startPosition - 1;
                dataLine = dataString.at(startPosition);
                for (i = 1; i <= lengthDataLine - 1; i++) dataLine = dataLine + dataString.at(startPosition + i);
                mData[qE-1] = stod(dataLine);
                sP = false; eP = false;
            }
            t++;
    }

    int s = 1;

    for(i = 0; i <= 21; i++) {
        s = s + i;
        for (j = 0; j <= i; j++) {
            if (zValue)
                m[ClimatData::mHLevel21[i]][ClimatData::mHLevel21[j]].z = mData[s + j - 1];
            else
                m[ClimatData::mHLevel21[i]][ClimatData::mHLevel21[j]].m = mData[s + j - 1];
        }
    }
    delete [] mData;
}

