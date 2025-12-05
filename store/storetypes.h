#ifndef STORETYPES_H
#define STORETYPES_H

#include "../clients/AbstractConvertedData.h"
#include "../clients/convertdata.h"
#include "mainwindow.h"
#include <string.h>
//#include <QtGlobal>

enum Latitude {
    NORTH = 0,
    SOUTH
};
enum Longitude {
    EASTERN = 0,
    WESTERN
};

class ComplexCoordinates : public AbstractAnswerConvertedData {
public:
    ComplexCoordinates() : AbstractAnswerConvertedData(SIZEOF) {}
    unsigned char* dataCore() const {
//        Q_ASSERT(SIZEOF == sizeof(ComplexCoordinatesData));
        memcpy(m_data, &m_d, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        ComplexCoordinatesData *t = reinterpret_cast<ComplexCoordinatesData *>(dataCore());
        ConvertData::convertBytes(&t->degrees);
        return m_data;
    }
    struct ComplexCoordinatesData {
        int degrees;
        unsigned char minutes;
        unsigned char seconds;
        unsigned char type;
    };
    ComplexCoordinatesData m_d;
    static const int SIZEOF = 7;
};

class ComplexPosition : public AbstractAnswerConvertedData {
public:
    ComplexPosition() : AbstractAnswerConvertedData(SIZEOF) {}
    unsigned char* dataCore() const {
        memcpy(m_data, &m_d, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        ComplexPositionData *t = reinterpret_cast<ComplexPositionData *>(dataCore());
        ConvertData::convertBytes(&t->mdu);
        ConvertData::convertBytes(&t->tilt);
        ConvertData::convertBytes(&t->height);
        ConvertData::convertBytes(&t->tangage);
        return m_data;
    }
    struct ComplexPositionData {
        short mdu;
        short tilt;
        short height;
        short tangage;
    };
    ComplexPositionData m_d;
    static const int SIZEOF = 8;
};

struct DateTime : public AbstractAnswerConvertedData {
    DateTime() : AbstractAnswerConvertedData(SIZEOF) {}
    unsigned char* dataCore() const {
        memcpy(m_data, &utcTime, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        DateTime *t = reinterpret_cast<DateTime *>(dataCore());
        ConvertData::convertBytes(&t->utcTime);
        return m_data;
    }
    int utcTime;
    char utcOffset;
    static const int SIZEOF = 5;
};

struct DateTimeAmsT : public AbstractAnswerConvertedData {
    DateTimeAmsT() : AbstractAnswerConvertedData(SIZEOF) {}
    unsigned char* dataCore() const {
        memcpy(m_data, &utcTime, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        DateTime *t = reinterpret_cast<DateTime *>(dataCore());
        ConvertData::convertBytes(&t->utcTime);
        return m_data;
    }
    char utcTime[15];
    char utcOffset;
    static const int SIZEOF = 16;
};

struct ArbitaryHeight : public AbstractAnswerConvertedData {
    ArbitaryHeight() : AbstractAnswerConvertedData(SIZEOF) {}
    unsigned char* dataCore() const {
        memcpy(m_data, &arbitraryHeight, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        ArbitaryHeight *t = reinterpret_cast<ArbitaryHeight *>(dataCore());
        ConvertData::convertBytes(&t->arbitraryHeight);
        return m_data;
    }
    int arbitraryHeight;
    static const int SIZEOF = 4;
};

struct Meteo11 : public AbstractAnswerConvertedData, public AbstractRequestConvertData {
    Meteo11() : AbstractAnswerConvertedData(0) {}
    static const int LENGTH = 251;
    char body[LENGTH];
protected:
    unsigned char* dataCore() const {
        return reinterpret_cast<unsigned char*>(const_cast<char*>(body));
    }
    unsigned char* convertedDataCore() const {
        return dataCore();
    }
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(body, data, size);
    }
    void convertDataCore() {}
};

struct FormalizeMeteo11 : public AbstractRequestConvertData {
    FormalizeMeteo11() {
        char *ptr = data;
        numStation = (int*)ptr;
        ptr += 4;
        day = ptr;
        ptr ++;
        hour = ptr;
        ptr ++;
        tensMin = ptr;
        ptr++;
        height = (short*)ptr;
        ptr += 2;
        pressure = (short*)ptr;
        ptr += 2;
        temperature = ptr;
        ptr++;
        meteoLevels = (MeteoLevel*)ptr;
        ptr += 19*sizeof(MeteoLevel);
        ht = ptr;
        ptr ++;
        hw = ptr;
    }

    struct MeteoLevel {
        char pp, tt, dd, ss;
    };
    int *numStation;
    char *day, *hour, *tensMin;
    short *height, *pressure;
    char *temperature;
    MeteoLevel *meteoLevels;
    char *ht, *hw;
    static const int SIZEOF = 14 + 19*sizeof(MeteoLevel);
    static const int NUM_LEVELS = 19;
    char data[SIZEOF];

    // AbstractRequestConvertData interface
protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(this->data, data, size);
    }
    void convertDataCore() {
        ConvertData::convertBytes(numStation);
        ConvertData::convertBytes(height);
        ConvertData::convertBytes(pressure);
    }
};



typedef float TWind;

struct WindData {
    WindData(int length) {
        direction = new TWind[length];
        speed = new TWind[length];
        height = new int[length];
        d_length = length;
    }
    ~WindData() {
        delete [] *direction;
        delete [] *speed;
        delete [] height;
    }
    void setRealWindHeights() {
        int plus = 25;
        for (int i = 0; i < d_length; i++) {
            if (i == 0)
                height[i] = 50;
            else {
                if (height[i - 1] == 100)
                    plus = 50;
                else if (height[i - 1] == 200)
                    plus = 100;
                else if (height[i - 1] == 1000)
                    plus = 200;
                height[i] = height[i - 1] + plus;
            }

        }
    }

    void setAverageWindHeights() {
        int plus = 50;
        for (int i = 0; i < d_length; i++) {
            if (i == 0)
                height[i] = 50;
            else {
                if (height[i - 1] == 100)
                    plus = 100;
                else if (height[i - 1] == 200)
                    plus = 200;
                else if (height[i - 1] == 800)
                    plus = 400;
                else if (height[i - 1] == 2400)
                    plus = 200;
                else if (height[i - 1] == 2600)
                    plus = 400;
                else if (height[i - 1] == 3000)
                    plus = 1000;
                else if (height[i - 1] == 6000)
                    plus = 2000;
                height[i] = height[i - 1] + plus;
            }
        }
    }

    int dataSize() {
        return d_length * sizeof(TWind);
    }
    int length() {
        return d_length;
    }
    struct WindParamPtr : public AbstractRequestConvertData { //обертка для данных
        WindParamPtr() : m_size(0) {}
        TWind *ptr;
        TWind* operator*() {return ptr;}
        WindParamPtr& operator=(TWind *d) {
            ptr = d;
            return *this;
        }
        TWind& operator[](int i) {
            return ptr[i];
        }
    protected:
        void setDataCore(unsigned char *data, unsigned int size) {
            memcpy(ptr, data, size);
            m_size = size;
        }
        void convertDataCore() {
            for (unsigned int i = 0; i < m_size; i++) {
                ConvertData::convertBytes(&ptr[i]);
            }
        }
        unsigned int m_size;
    private:
        void *operator new(std::size_t size) {(void)size; return nullptr;}
        void *operator new[](std::size_t size) {(void)size; return nullptr;}
        void operator delete(void *p) {(void)p;}
        void operator delete[](void *p) {(void)p;}
    };

    WindParamPtr direction;
    WindParamPtr speed;
    int *height;
    static const int AVERAGE_WIND_LENGTH = 16;
    static const int REAL_WIND_LENGTH = 30;
private:
    int d_length;

};
struct IzmWindData : public AbstractRequestConvertData {
    struct ProfilePoint {
        float speed;
        float azimut;
        float height;
        int curr;
    };
    static const int LENGTH = 320;

    static int lenghtForMode(TargetMode mode){
        return (mode == TargetMode::AMS) ? 100 : 320;
    }

    static const int SIZEOF = 5120;

    static int sizeofForMode(TargetMode mode){
        return (mode == TargetMode::AMS) ? 1600 : 5120;
    }

    ProfilePoint data[LENGTH];

    // AbstractRequestConvertData interface
protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(this->data, data, size);
    }
    void convertDataCore() {
        for (int i = 0; i < LENGTH; i++) {
            ConvertData::convertBytes(&data[i].speed);
            ConvertData::convertBytes(&data[i].azimut);
            ConvertData::convertBytes(&data[i].height);
            ConvertData::convertBytes(&data[i].curr);
        }
    }
};

struct WindHData : public AbstractRequestConvertData {
    struct ProfilePoint {
        float speedH;
        float azimutH;
        float heightH;
    };
    static const int LENGTH = 40;
    static const int SIZEOF = 480;
    ProfilePoint data[LENGTH];

    // AbstractRequestConvertData interface
protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(this->data, data, size);
    }
    void convertDataCore() {
        for (int i = 0; i < LENGTH; i++) {
            ConvertData::convertBytes(&data[i].speedH);
            ConvertData::convertBytes(&data[i].azimutH);
            ConvertData::convertBytes(&data[i].heightH);
        }
    }
};

struct WeatherParameters1B65 : public AbstractAnswerConvertedData, public AbstractRequestConvertData {
    WeatherParameters1B65() : AbstractAnswerConvertedData(SIZEOF) {}
    struct WeatherParameters1B65Data {
        float atmospherePressure;
        float airTemprature;
        float relativeHumidity;
        float windDirection;
        float windSpeed;
    };
    WeatherParameters1B65Data m_d;
    static const int SIZEOF = 20;

    // AbstractRequestConvertData interface
protected:
    unsigned char* dataCore() const {
        memcpy(m_data, &m_d, SIZEOF);
        return m_data;
    }
    unsigned char* convertedDataCore() const {
        WeatherParameters1B65Data *t = reinterpret_cast<WeatherParameters1B65Data *>(dataCore());
        ConvertData::convertBytes(&t->atmospherePressure);
        ConvertData::convertBytes(&t->airTemprature);
        ConvertData::convertBytes(&t->relativeHumidity);
        ConvertData::convertBytes(&t->windDirection);
        ConvertData::convertBytes(&t->windSpeed);
        return m_data;
    }
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(&m_d, data, size);
    }
    void convertDataCore() {
        ConvertData::convertBytes(&m_d.atmospherePressure);
        ConvertData::convertBytes(&m_d.airTemprature);
        ConvertData::convertBytes(&m_d.relativeHumidity);
        ConvertData::convertBytes(&m_d.windDirection);
        ConvertData::convertBytes(&m_d.windSpeed);
    }
};
struct StationState : public AbstractRequestConvertData {

    short state, sondingState, errorState;
    int errorRpv;
    float error1b65b;
    int numComplexOn, numClictronOn;
    static const int SIZEOF = 22;

    // AbstractRequestConvertData interface
protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        (void)size;
        short *s = reinterpret_cast<short*>(data);
        state = s[0];
        sondingState = s[1];
        errorState = s[2];
        int *i = reinterpret_cast<int*>(data + 6);
        errorRpv = i[0];
        error1b65b = reinterpret_cast<float*>(data + 10)[0];
        numComplexOn = i[2];
        numClictronOn = i[3];

//        Q_ASSERT(sizeof(StationState) == 24);       //
//        char* stv = (char*)st;                      //
//        char c[24];                                 //
//        memcpy(c, stv, 24);                         //
//        memset(stv, 0, 24);                         //
//        memcpy(stv, c, 6);                          //
//        memcpy(stv+8, c + 6, 16);                   // заглушка от выравнивания
    }

    void convertDataCore() {
        ConvertData::convertBytes(&state);
        ConvertData::convertBytes(&sondingState);
        ConvertData::convertBytes(&errorState);
        ConvertData::convertBytes(&errorRpv);
        ConvertData::convertBytes(&error1b65b);
        ConvertData::convertBytes(&numComplexOn);
        ConvertData::convertBytes(&numClictronOn);
    }
};

struct SondingStartAnswer : public AbstractRequestConvertData {
    char sondingStart;
//    SondingStartAnswer& operator=(char sondingStart) {
//        this->sondingStart = sondingStart;
//        return *this;
//    }
    // AbstractRequestConvertData interface
protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        (void)size;
        sondingStart = *data;
    }
    void convertDataCore() {}
};


//---
struct ArbitraryHeightWind{
    int resultCalc;
    int arbitraryHeight;
    float windVelocity;
    float windDirection;
    float averageWindVelocity;
    float averageWindDirection;
    static const int SIZEOF = 24;
};
//---

struct Meteo11Updated : public AbstractRequestConvertData {
    Meteo11Updated() {
        memset(Meteo11UpdatedData, 0, SIZEOF);
        initializeData();
    }

    void initializeData(){
        char *ptr = Meteo11UpdatedData;

        dateTime = new char[15];
        memcpy(dateTime, ptr, 15); // Первые 15 байт для строки dateTime
        ptr += 15;

        utcOffset = *ptr;  // Смещение UTC
        ptr++;

        avgTime = *ptr;  // Среднее время
        ptr++;

        degree1 = *reinterpret_cast<int*>(ptr);  // Угол 1
        ptr += sizeof(int);
        minutes1 = *ptr;  // Минуты 1
        ptr++;
        seconds1 = *ptr;  // Секунды 1
        ptr++;
        longtitude1 = *ptr;  // Долгота 1
        ptr++;

        degree2 = *reinterpret_cast<int*>(ptr);  // Угол 2
        ptr += sizeof(int);
        minutes2 = *ptr;  // Минуты 2
        ptr++;
        seconds2 = *ptr;  // Секунды 2
        ptr++;
        latitude1 = *ptr;  // Широта 1
        ptr++;

        degree3 = *reinterpret_cast<int*>(ptr);  // Угол 3
        ptr += sizeof(int);
        minutes3 = *ptr;  // Минуты 3
        ptr++;
        seconds3 = *ptr;  // Секунды 3
        ptr++;
        longtitude2 = *ptr;  // Долгота 2
        ptr++;

        degree4 = *reinterpret_cast<int*>(ptr);  // Угол 4
        ptr += sizeof(int);
        minutes4 = *ptr;  // Минуты 4
        ptr++;
        seconds4 = *ptr;  // Секунды 4
        ptr++;
        latitude2 = *ptr;  // Широта 2
        ptr++;

        lastMinutes = *reinterpret_cast<int*>(ptr);  // Последние минуты
        ptr += sizeof(int);
        sondingDistance = *reinterpret_cast<int*>(ptr);  // Расстояние зондирования
        ptr += sizeof(int);
        dataStatus = *ptr;  // Статус данных
        ptr++;

        secondaryDateTime = new char[15];
        memcpy(secondaryDateTime, ptr, 15);// Вторые 15 байт для secondaryDateTime
        ptr += 15;
        secondaryUtcOffset = *ptr;  // Смещение UTC для secondaryDateTime
        ptr++;

        meteoText = ptr;  // Текстовые данные
    }

    char *dateTime;
    char utcOffset;
    char avgTime;

    int degree1;
    char minutes1;
    char seconds1;
    char longtitude1;

    int degree2;
    char minutes2;
    char seconds2;
    char latitude1;

    int degree3;
    char minutes3;
    char seconds3;
    char longtitude2;

    int degree4;
    char minutes4;
    char seconds4;
    char latitude2;

    int lastMinutes;
    int sondingDistance;
    char dataStatus;

    char *secondaryDateTime;
    char secondaryUtcOffset;

    char *meteoText;

    static const int SIZEOF = 321;
    char Meteo11UpdatedData[SIZEOF];

protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(this->Meteo11UpdatedData, data, size);
        initializeData();
    }

    void convertDataCore() {
        ConvertData::convertBytes(&degree1);
        ConvertData::convertBytes(&degree2);
        ConvertData::convertBytes(&degree3);
        ConvertData::convertBytes(&degree4);
        ConvertData::convertBytes(&lastMinutes);
        ConvertData::convertBytes(&sondingDistance);
    }
};

struct FormalizeMeteo11Updated : public AbstractRequestConvertData {
    FormalizeMeteo11Updated() {
        memset(Meteo11FormalizeUpdatedData, 0, SIZEOF);
        initializeData();
    }

    void initializeData(){
        char *ptr = Meteo11FormalizeUpdatedData;

        dateTimeForm = new char[15];
        memcpy(dateTimeForm, ptr, 15); // Первые 15 байт для строки dateTime
        ptr += 15;

        utcOffsetForm = *ptr;  // Смещение UTC
        ptr++;

        avgTimeForm = *ptr;  // Среднее время
        ptr++;

        degree1Form = *reinterpret_cast<int*>(ptr);  // Угол 1
        ptr += sizeof(int);
        minutes1Form = *ptr;  // Минуты 1
        ptr++;
        seconds1Form = *ptr;  // Секунды 1
        ptr++;
        longtitude1Form = *ptr;  // Долгота 1
        ptr++;

        degree2Form = *reinterpret_cast<int*>(ptr);  // Угол 2
        ptr += sizeof(int);
        minutes2Form = *ptr;  // Минуты 2
        ptr++;
        seconds2Form = *ptr;  // Секунды 2
        ptr++;
        latitude1Form = *ptr;  // Широта 1
        ptr++;

        degree3Form = *reinterpret_cast<int*>(ptr);  // Угол 3
        ptr += sizeof(int);
        minutes3Form = *ptr;  // Минуты 3
        ptr++;
        seconds3Form = *ptr;  // Секунды 3
        ptr++;
        longtitude2Form = *ptr;  // Долгота 2
        ptr++;

        degree4Form = *reinterpret_cast<int*>(ptr);  // Угол 4
        ptr += sizeof(int);
        minutes4Form = *ptr;  // Минуты 4
        ptr++;
        seconds4Form = *ptr;  // Секунды 4
        ptr++;
        latitude2Form = *ptr;  // Широта 2
        ptr++;

        lastMinutesForm = *reinterpret_cast<int*>(ptr);  // Последние минуты
        ptr += sizeof(int);
        sondingDistanceForm = *reinterpret_cast<int*>(ptr);  // Расстояние зондирования
        ptr += sizeof(int);
        dataStatusForm = *ptr;  // Статус данных
        ptr++;

        secondaryDateTimeForm = new char[15];
        memcpy(secondaryDateTimeForm, ptr, 15);// Вторые 15 байт для secondaryDateTime
        ptr += 15;
        secondaryUtcOffsetForm = *ptr;  // Смещение UTC для secondaryDateTime
        ptr++;

        numStationForm = (int*)ptr;
        ptr += 4;
        dayForm = ptr;
        ptr ++;
        hourForm = ptr;
        ptr ++;
        tensMinForm = ptr;
        ptr++;
        heightForm = (short*)ptr;
        ptr += 2;
        pressureForm = (short*)ptr;
        ptr += 2;
        temperatureForm = ptr;
        ptr++;
        meteoLevelsForm = (MeteoLevelsForm*)ptr;
        ptr += 19*sizeof(MeteoLevelsForm);
        htForm = ptr;
        ptr ++;
        hwForm = ptr;
    }

    struct MeteoLevelsForm{
        char pp1, tt1, dd1, ss1;
    };

    char *dateTimeForm;
    char utcOffsetForm;
    char avgTimeForm;

    int degree1Form;
    char minutes1Form;
    char seconds1Form;
    char longtitude1Form;

    int degree2Form;
    char minutes2Form;
    char seconds2Form;
    char latitude1Form;

    int degree3Form;
    char minutes3Form;
    char seconds3Form;
    char longtitude2Form;

    int degree4Form;
    char minutes4Form;
    char seconds4Form;
    char latitude2Form;

    int lastMinutesForm;
    int sondingDistanceForm;
    char dataStatusForm;

    char *secondaryDateTimeForm;
    char secondaryUtcOffsetForm;

    int *numStationForm;
    char *dayForm, *hourForm, *tensMinForm;
    short *heightForm, *pressureForm;
    char *temperatureForm;
    MeteoLevelsForm *meteoLevelsForm;
    char *htForm, *hwForm;

    static const int SIZEOF = 91 + 19*sizeof(MeteoLevelsForm);
    static const int NUM_LEVELSUPD = 19;
    char Meteo11FormalizeUpdatedData[SIZEOF];

protected:
    void setDataCore(unsigned char *data, unsigned int size) {
        memcpy(this->Meteo11FormalizeUpdatedData, data, size);
        initializeData();
    }

    void convertDataCore() {
        ConvertData::convertBytes(&degree1Form);
        ConvertData::convertBytes(&degree2Form);
        ConvertData::convertBytes(&degree3Form);
        ConvertData::convertBytes(&degree4Form);
        ConvertData::convertBytes(&lastMinutesForm);
        ConvertData::convertBytes(&sondingDistanceForm);
        ConvertData::convertBytes(numStationForm);
        ConvertData::convertBytes(heightForm);
        ConvertData::convertBytes(pressureForm);
    }
};

#endif
