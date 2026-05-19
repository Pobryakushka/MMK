#include "InData.h"
#include "stdlib.h"

#include <algorithm>
#include <iostream>
#include <fstream>

#include "Subsidiary/fileprocessing.h"
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"
//#include "FunctionsCalc/functionsCalc.h"


#include <iostream>

using namespace std;

FileProcessing fileProcessing;
//double InData::B=/*55.93*//*41.4*//*53.08*//*59.95*//*39.95*//*55.00*/0;
//double InData::L=/*37.52*//*69.36*//*158.58*//*30.7*//*32.88*//*37.00*/0;
double InData::H=/*190*//*492*//*84*//*76*//*891*//*120*/-10001;

////Чита
//double InData::B=52.08;
//double InData::L=113.48;
//double InData::H=671;

////Анадырь
//double InData::B=64.73;
//double InData::L=177.53;
//double InData::H=6.0;

////Котельный
//double InData::B=76.00;
//double InData::L=137.86;
//double InData::H=15.0;

////Салехард
//double InData::B=66.53;
//double InData::L=66.67;
//double InData::H=16.0;

////Тегеран
//double InData::B=35.68;
//double InData::L=51.32;
//double InData::H=1191.0;

////Берген
//double InData::B=/*52.82*/54.1;
//double InData::L=/*9.92*/13.01;
//double InData::H=/*78.0*/6;

int InData::Tau0::GD0=0;
int InData::Tau0::MS0=0;
int InData::Tau0::DN0=0;
double InData::Tau0::CH0=0;

double InData::z0=0.01;

double InData::h0=0/*4*/;
double InData::V0_tau0=0;
double InData::TETA0_tau0=0;


//const int InData::kk=18/*14*//*12*//*5*//*3*//*19*//*22*//*12*//*34*//*14*//*23*//*17*//*12*//*73*//*76*/;
//Анадырь, Котельный
//const int InData::kk=4;
////Чита
//const int InData::kk=/*10*//*15*//*9*/11;
//Салехард, Тегеран
//const int InData::kk=8;
//Берген
//const int InData::kk=18;

int InData::k=0;
bool InData::bottomLayer;
vector <PlowAlgoritm::ProfilePoint> InData::ProfRPV_table;
bool InData::prevZond=false;
OutData::V InData::V0ZM;
int InData::kprev=0;
std::vector <MBulletin> InData::MBulletin_table;
double InData::LimitTimeMB=12;//

InData::InData()
{
//    for (int i = 0; i < /*Constants::numMB*/kprev; i++) {
//        MBulletin_table[i].setupLayersCount(Constants::numStL);
//    }

    //InData::GetTau0();
    /*std::cout <<"Tau0: "<<InData::tau0.GD0<<" "<<InData::tau0.MS0<<" "<<InData::tau0.DN0<<" "<<InData::tau0.CH0<<"\r\n";
    std::cout <<"TauM: "<<InData::MBulletin_table[0].tauM.GD0<<" "<<InData::MBulletin_table[0].tauM.MS0<<" "<<InData::MBulletin_table[0].tauM.DN0<<" "<<InData::MBulletin_table[0].tauM.CH0<<"\r\n";*/

//    if (Get_MBulletin_table())
//        InData::prevZond=true;
//    else InData::prevZond=false;
}

bool InData::Get_MBulletin_table(MBulletin* MB, int kprev){
    try{
        InData::kprev=kprev;
        for (int i = 0; i < /*Constants::numMB*/kprev; i++) {
            MB[i].setupLayersCount(Constants::numStL);
        }
        char* mas[1000];
        for (int i=0;i<1000;i++)
            mas[i]=/*""*/NULL;
        char *pch;
        int j=0;
      //  std::cout<<"MeteoBulletins:\n";
        ReadProfMeteo(mas, InData::kprev);
        if (InData::kprev<=0)
            return false;
     //   double hour, minutes;
        int masd[43];
        double CH0=InData::GetCHfromDate(InData::tau0.GD0,InData::tau0.MS0,InData::tau0.DN0, InData::tau0.CH0);
        for (int i=0;i<InData::kprev;i++){
            j=0;
            pch=strtok(mas[i]," \t\r");
                    while(pch!=NULL)
                    {
                     std::replace(pch,pch+strlen(pch),',','.');
                     masd[j]=atoi(pch);
                     pch = strtok (NULL," \t\r");
                     j++;
                    }
               /*InData::MBulletin_table*/MB[i].Hm=masd[0]*10;
               /*InData::MBulletin_table*/MB[i].tauM.DN0=masd[1];
               /*InData::MBulletin_table*/MB[i].tauM.MS0=masd[2];
               /*InData::MBulletin_table*/MB[i].tauM.GD0=masd[3]+2000;
               /*InData::MBulletin_table*/MB[i].tauM.CH0=Get_CH(masd[4],masd[5]);
               int st=0;
               for (int p=6;p<=39;p+=3){
                   /*InData::MBulletin_table*/MB[i].ProfMeteo[st].h=/*Climat::StandartLayer*/Constants::StandartLayerMB[st];
                   /*InData::MBulletin_table*/MB[i].ProfMeteo[st].TETA=masd[p+1]*6;
                   /*InData::MBulletin_table*/MB[i].ProfMeteo[st].V=masd[p+2];
                   st++;
               }
               /*InData::MBulletin_table*/MB[i].Hd=masd[42]*100;
               double CHM=InData::GetCHfromDate(/*InData::MBulletin_table*/MB[i].tauM.GD0,/*InData::MBulletin_table*/MB[i].tauM.MS0,
                                                 /*InData::MBulletin_table*/MB[i].tauM.DN0,/*InData::MBulletin_table*/MB[i].tauM.CH0);
               if ((CH0-CHM)>InData::LimitTimeMB)
                   /*InData::MBulletin_table*/MB[i].checkm=false;//негодный МБ
               else /*InData::MBulletin_table*/MB[i].checkm=true;
               //Для проверки правильного раскодирования метеобюллетеня
               char* ch=(char*)"false";
               if (/*InData::MBulletin_table*/MB[i].checkm)
                   ch="true";
               std::cout <</*InData::MBulletin_table*/MB[i].Hm<<" "<</*InData::MBulletin_table*/MB[i].tauM.DN0<<" "<<
               /*InData::MBulletin_table*/MB[i].tauM.MS0<<" "<</*InData::MBulletin_table*/MB[i].tauM.GD0<<" "<<
                    /*InData::MBulletin_table*/MB[i].tauM.CH0<<" "<<CH0-CHM<<" "<<ch<<"\r\n" ;
               for (int l=0;l</*Constants::numStL*/Constants::numStL_MB;l++)
               std::cout<</*InData::MBulletin_table*/MB[i].ProfMeteo[l].h<<" "
               <</*InData::MBulletin_table*/MB[i].ProfMeteo[l].TETA<<" "
               <</*InData::MBulletin_table*/MB[i].ProfMeteo[l].V<<"\r\n" ;
        }
        //std::cout <<"Tau0: "<<InData::tau0.GD0<<" "<<InData::tau0.MS0<<" "<<InData::tau0.DN0<<" "<<InData::tau0.CH0<<"\r\n";
      //  std::cout<<hour<<endl;
        return true;
    }
    catch(...){
        return false;
    }
}

//Считывает файл climatsi
void InData::ReadProfMeteo(char *mas[], int /*&*/i)
{
    char buf[1000];
    ifstream f("d:\\Luba\\2\\Plow\\debug\\Smile_m.txt", ios::in /*| ios::binary*/);
   if (!f)
   {
    cout<<"Error file Smile_m.txt open!";
    return ;
   }
   i=0;
   while (!f.eof())
   {
    f.getline(buf,256/*,'\r\n'*/);
    mas[i]=new char[strlen(buf)];//
    strcpy(mas[i],buf);
    i++;
   // system("pause");
   }
   f.close();//
}


//Получить параметры вертикального профиля ветра
bool InData::Get_ProfRPV_table(PlowAlgoritm::ProfilePoint* profTest, int k, char* fileName)
 {
    try{
    // ProfRPV_table.reserve(InData::k);
    // ProfRPV_table.resize(InData::k);
     char *mas[/*InData::k*/k];
     char *pch;
     int j=0;

     std::cout<<"VECTOR Vertikalniy profil vetra:\n";
     if (!fileProcessing.ReadRPVprof(/*InData::k*/k, mas, fileName))
         return false;

     for (int i=0; i</*InData::k*/k; i++){
         j=0;
         pch=strtok(mas[i],"\t\r");
                 while(pch!=NULL)
                 {
                    std::replace(pch,pch+strlen(pch),',','.');
                     switch (j){
                     case 0:{
                         //ProfRPV_table[i].height=atof(pch);
                         profTest[i].height=atof(pch);
                         break;}
                     case 1:
                          //ProfRPV_table[i].speed=atof(pch) ;
                         profTest[i].speed=atof(pch) ;
                         break;
                     case 2:{
                          // ProfRPV_table[i].azimut=atof(pch) ;
                          profTest[i].azimut=atof(pch) ;
                         break;}
                     default:
                         break;
                     }
                  pch = strtok (NULL,"\t\r");
                  j++;
                 }
                // ProfRPV_table[i].curr=1;
                  profTest[i].curr=1;
                 //std::cout <<ProfRPV_table[i].height<<"\t"<<ProfRPV_table[i].speed<<"\t"<<ProfRPV_table[i].azimut<<"\r\n" ;
                  std::cout <<profTest[i].height<<"\t"<<profTest[i].speed<<"\t"<<profTest[i].azimut<<"\r\n" ;
                 delete [] mas[i];

     }

     return true;
    }
    catch(...)
    {
     return false;
    }
 }




