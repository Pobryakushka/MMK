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

int kpr=94;//Кол-во уровней измерения РПВ
int kprev=4;//Кол-во предыдущих зондирований (кол-во МБ)
bool prevzond=false/*true*/;//Признак учета (true)или неучета(false)предыдущих зондирований
float z0=0.01;//Параметр шероховатости, м

double B=/*54.1*//*55.93*//*41.4*//*53.08*//*59.95*//*39.95*/55.00;//широта
double L=/*13.01*//*37.52*//*69.36*//*158.58*//*30.7*//*32.88*/37.00;//долгота
double H=/*6*//*190*//*492*//*84*//*76*//*891*/120;//высота

//Дата
int GD0=2008/*2016*/;
int MS0=8/*1*/;
int DN0=26/*20*/;
float CH0=9.717/*11*/;

//Приземный ветер
float speed=5;//м/c
float azimut=20;//град
float height=4;//м

//const int kpr=/*14*//*0*/94/*38*//*96*//*86*//*60*/;
//const int kpr=18/*14*//*5*//*3*//*19*//*22*//*12*//*73*//*34*//*14*//*23*//*17*//*76*/;
//const int kpr=/*10*//*15*//*9*/11;//Чита
//const int kpr=4;//Анадырь, Котельный
//const int kpr=8;//Салехард, Тегеран
//const int kpr=18;//Берген
//float winds[kpr*2];
float* winds=new float[kpr*2];//Сдвиг ветра

InData inData;
ProfileReal profile/*(realWind,averageWind)*/;
WindShear windShear(winds);

//ToScreen(ProfileReal::MessErrors Rez);

void ToScreen(ProfileReal::MessErrors Rez){
    std::cout<<"-------------------------" <<std::endl;
    switch (Rez) {
    case 0:{
        std::cout<<"Ok! GetProfile"<<std::endl;
        break;
    }
    case 1:{
        std::cout<<"Error!!! GetProfile"<<std::endl;
        break;
    }
    default:
        break;
    }

    std::cout<<"-------------------------" <<std::endl;
}

void ToScreenWS(WindShear::MessErrors Rez){
    std::cout<<"-------------------------" <<std::endl;
    switch (Rez) {
    case 0:{
        std::cout<<"Ok! GetWindShear"<<std::endl;
        break;
    }
    case 1:{
        std::cout<<"Error!!! GetWindShear"<<std::endl;
        break;
    }
    default:
        break;
    }

    std::cout<<"-------------------------" <<std::endl;
}

int main()
{
    cout << "Hello, World!" << endl;
          //------------------------
          ClimatData *cd = new ClimatData("d:\\Luba\\Tasks\\Zemledelie\\Zemledelie_new_14_06_2017\\EMA\\dat\\climat\\");
          PlowAlgoritm::ProfilePoint* profTest=new PlowAlgoritm::ProfilePoint[kpr];
          inData.Get_ProfRPV_table(profTest, kpr,"d:\\Luba\\2\\Plow\\debug\\profRPV_m.txt");
          MBulletin* MB=new MBulletin[kprev];
          inData.Get_MBulletin_table(MB, kprev);
       /* for(int i=0;i<2;i++)*/{
          //-------------------------
          profile.setGroundWind(/*speed*/0.9, /*azimut*/16.4, /*height*/4);
//          profile.setBLH(B,L,H);
          profile.setDateTime(GD0, MS0, DN0, CH0);
          profile.setProfRpv(profTest, kpr);
          profile.setMeteoMid(MB, kprev, prevzond);
        //  profile.setMeteoMid(MB, kprev, false);
          cd->setClZone(B, L, MS0);
          profile.setClimatData(cd);
          profile.setz0(z0);

          ProfileReal::MessErrors Rez=profile.GetProfile( realWind, averageWind);

          //===========================================================
          inData.Get_ProfRPV_table(profTest, kpr,"d:\\Luba\\2\\Plow\\debug\\profRPV_m.txt");
          profile.setGroundWind(speed, azimut, height);
//          profile.setBLH(B,L,H);
          profile.setDateTime(GD0, MS0, DN0, CH0);
          profile.setProfRpv(profTest, kpr);
          profile.setMeteoMid(MB, kprev, prevzond);
        //  profile.setMeteoMid(MB, kprev, false);
          cd->setClZone(B, L, MS0);
          profile.setClimatData(cd);
          profile.setz0(z0);

          /*ProfileReal::MessErrors*/ Rez=profile.GetProfile( realWind, averageWind);
          //===========================================================


         // Rez=profile.GetProfile( realWind, averageWind);
          ToScreen(Rez);

          std::cout<<"--------realWind---------" <<std::endl;
          for(int i=0;i<Constants::REAL_NUM_DATA;i++)
             std::cout<<realWind[i+Constants::REAL_NUM_DATA]<<"\t"<<realWind[i]<<std::endl;
          std::cout<<"--------averageWind---------" <<std::endl;
          for(int i=0;i<Constants::LOW_NUM_DATA;i++)
             std::cout<<averageWind[i+Constants::REAL_NUM_DATA]<<"\t"<<averageWind[i]<<std::endl;

          windShear.setProfRpv(profTest,kpr);
          WindShear::MessErrors RezWS=windShear.GetWindShear(winds);
          ToScreenWS(RezWS);
          std::cout<<std::endl;
          std::cout<<"-----------Sdvig Vetra-----------"<<std::endl;
          for(int i=0;i<(kpr-1);i++)
            std::cout<<winds[i+(kpr-1)]<<"\t\t"<<winds[i]<<std::endl;

       }
        delete[] profTest;
        delete[] MB;


        return 0;
}