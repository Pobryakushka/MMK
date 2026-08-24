#include <iostream>

#include "InData/indata.h"
#include "FunctionsCalc/mathfunc.h"
#include "Profile/profile.h"

#include "stdlib.h"
#include "stdio.h"
#include "WindShear/windshear.h"
#include <climatdata.h>
using namespace std;//

float averageWind[Constants::LOW_NUM_DATA*2];
float realWind[Constants::REAL_NUM_DATA * 2];

// ── ИЗМЕНЕНО: имитируем дежурный режим АМС (нет измерений) ──
int kpr=0;                 // ← было 94, теперь 0 — НЕТ измерений локатора
int kprev=0;                // ← было 4, теперь 0 — НЕТ метеобюллетеней
bool prevzond=false;        // как и было — бюллетени не учитываем
float z0=0.01;

// ── ИЗМЕНЕНО: реальные координаты станции из вашего сценария (Москва) ──
double B=55.7558;   // широта
double L=37.6172;   // долгота
double H=156;        // высота станции, м

// ── ИЗМЕНЕНО: реальная дата/месяц из вашего сценария (август) ──
int GD0=2026;
int MS0=8;
int DN0=21;
float CH0=12.0;

// ── ИЗМЕНЕНО: реальный наземный ветер из вашего сценария ──
float speed=13;    // м/c
float azimut=311;  // град
float height=10;   // высота датчика IWS, м — уточните, если другая

InData inData;
ProfileReal profile;
// WindShear здесь не тестируем — закомментировано, т.к. без измерений
// сдвиг ветра не считается в принципе
// float* winds=new float[1];
// WindShear windShear(winds);

void ToScreen(ProfileReal::MessErrors Rez){
    std::cout<<"-------------------------" <<std::endl;
    switch (Rez) {
    case 0:  std::cout<<"Ok! GetProfile"<<std::endl; break;
    case 1:  std::cout<<"Error!!! GetProfile (PROFILE_NOT_CONFIG_PROF_RPV)"<<std::endl; break;
    case 2:  std::cout<<"Error!!! GetProfile (PROFILE_CLIMAT_NOT_READY)"<<std::endl; break;
    case 3:  std::cout<<"Error!!! GetProfile (PROFILE_NOT_CONFIG_HEIGHT)"<<std::endl; break;
    default: std::cout<<"Unknown result code: "<<Rez<<std::endl; break;
    }
    std::cout<<"-------------------------" <<std::endl;
}

int main()
{
    cout << "Hello, World! (ТЕСТ ДЕЖУРНОГО РЕЖИМА, k=0)" << endl;

    // ── ИЗМЕНЕНО: путь к climat-данным — Linux-путь вместо d:\Luba\... ──
    // Подставьте реальный путь, который использует MMK:
    ClimatData *cd = new ClimatData(
        "/home/otdel412/Projects/build-MMK-Desktop-Debug/climatData/climat/");

    // ── ИЗМЕНЕНО: НЕ читаем profRPV_m.txt, НЕ читаем метеобюллетени ──
    // Эмулируем пустой профиль измерений (как setProfRpv делает WindProfileCalculator
    // при h=0): передаём hCount=0, указатель может быть nullptr — цикл внутри
    // setProfRpv() с hCount=0 не разыменовывает его.
    PlowAlgoritm::ProfilePoint* profTest = nullptr;

    profile.setGroundWind(speed, azimut, height);
    profile.setDateTime(GD0, MS0, DN0, CH0);
    profile.setProfRpv(profTest, kpr);        // kpr=0 → InData::k=0
    profile.setMeteoMid(nullptr, 0, false);   // явно: нет бюллетеней
    profile.setHeight(H);                      // ВАЖНО: в исходном тесте
                                                 // setHeight/setBLH не вызывались!
    cd->setClZone(B, L, MS0);
    profile.setClimatData(cd);
    profile.setz0(z0);

    std::cout << "[TEST] isDataReady=" << cd->isDataReady()
              << " файл=" << cd->dataFileName()
              << " станция=" << cd->stationOrderNumber()
              << " расстояние=" << cd->rangeToCloserStation()
              << " зона=" << cd->getCurrentClZone().numClZone
              << std::endl;

    ProfileReal::MessErrors Rez = profile.GetProfile(realWind, averageWind);
    ToScreen(Rez);

    std::cout<<"--------realWind---------" <<std::endl;
    for(int i=0;i<Constants::REAL_NUM_DATA;i++)
        std::cout<<realWind[i+Constants::REAL_NUM_DATA]<<"\t"<<realWind[i]<<std::endl;
    std::cout<<"--------averageWind---------" <<std::endl;
    for(int i=0;i<Constants::LOW_NUM_DATA;i++)
        std::cout<<averageWind[i+Constants::REAL_NUM_DATA]<<"\t"<<averageWind[i]<<std::endl;

    delete cd;
    return 0;
}
