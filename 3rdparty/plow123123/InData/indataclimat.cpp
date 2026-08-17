#include "indataclimat.h"
#include "Subsidiary/fileprocessing.h"

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "stdlib.h"
#include "string.h"
#include "InData/Constants.h"

//Climat::Sigma Climat::ClimatSi[2][11][12][12];
//OutData::V Climat::ClimatPr[2][11][12][12];
Climat::Correl Climat::ClimatCorrel[11][12][11][12];
double Climat::StandartLayer[/*15*/Climat::numStL]=
 { 100.0, 200.0, 400.0, 600.0, 800.0, 1200.0, 1600.0,
   2000.0, 2400.0, 2600.0, 3000.0, 4000.0, 5000.0, 6000.0, 8000.0};

int Climat::Zone=Climat::Get_Zone();

using namespace std;
Climat::Climat()
{
   Get_ClimatSi();
   Get_ClimatPr();
   Get_ClimatCorrel();
}

//Получить значения среднеквадратических отклонений зональной и меридиональной составляющих
//среднего ветра для стандартного слоя из климатической БД
bool Climat::Get_ClimatSi()
 {
//    try{
//        char* mas[1000];
//        for (int i=0;i<1000;i++)
//            mas[i]='\0';
//        char *pch;

//        std::cout<<"ClimatSi:\n";
//        ReadClimatSi(mas);
//        int ip=0;
//        int iz=0;
//        int is=0;
//        int im=0;
//        bool iYM=false;
//        for (int i=0;i<1000;i++){
//          if (mas[i]=='\0')
//             break;
//         // std::cout <<mas[i]<<"\r\n" ;
//          pch=strtok(mas[i],"\t\r");
//          while(pch!=NULL){
//              std::replace(pch,pch+strlen(pch),',','.');
//              if (!iYM) /*false*/ {
//                  Climat::ClimatSi[ip][iz][is][im].SiVZ=atof(pch);
//              }
//              else /*true*/{
//                  Climat::ClimatSi[ip][iz][is][im].SiVM=atof(pch);
//              }
//              pch = strtok (NULL,"\t\r");
//              im++;
//              if (im>11)
//                  im=0;
//          }
//          iYM=!iYM;
//          if (iYM==0) is++;
//          if (is>11){
//              is=0;
//              iz+=1;
//          }
//          if (iz>10){
//              iz=0;
//              ip+=1;
//          }

//        }
//     // std::cout <<"VZ="<<Climat::ClimatSi[1][3][0][7].SiVZ<<"\r\n" ;
//     // std::cout <<"VM="<<Climat::ClimatSi[1][3][0][7].SiVM<<"\r\n" ;

//        return true;
//    }
//    catch(...)
//    {
//        return false;
//    }
 }

//Получить среднеклиматические значения параметров ветра
//из климатической БД
bool Climat::Get_ClimatPr()
 {
//    try{
//        char* mas[1000];
//        for (int i=0;i<1000;i++)
//            mas[i]="";
//        char *pch;
//        int j=0;

//        std::cout<<"ClimatPr:\n";
//        ReadClimatPr(mas);
//        int ip=0;
//        int iz=0;
//        int is=0;
//        int im=0;
//        bool iYM=false;
//        for (int i=0;i<1000;i++){
//          if (mas[i]=='\0')
//             break;
//         // std::cout <<mas[i]<<"\r\n" ;
//          pch=strtok(mas[i],"\t\r");
//          while(pch!=NULL){
//              std::replace(pch,pch+strlen(pch),',','.');
//              if (iYM) /*false*/{
//                  Climat::ClimatPr[ip][iz][is][im].VZ=atof(pch);
//                  break;
//              }
//              else /*true*/ {
//                  Climat::ClimatPr[ip][iz][is][im].VM=atof(pch);
//              }
//              pch = strtok (NULL,"\t\r");
//              im++;
//              if (im>11)
//                  im=0;
//          }
//          iYM=!iYM;
//          if (iYM==0) is++;
//          if (is>11){
//              is=0;
//              iz+=1;
//          }
//          if (iz>10){
//              iz=0;
//              ip+=1;
//          }

//        }
//        // std::cout <<"VZ="<<Climat::ClimatPr[0][1][2][3].VZ<<"\r\n" ;
//        //  std::cout <<"VM="<<Climat::ClimatPr[0][1][2][3].VM<<"\r\n" ;

//        return true;
//    }
//    catch(...)
//    {
//        return false;
//    }
 }

bool Climat::Get_ClimatCorrel()
 {
    try{
        char* mas[3000];
        for (int i=0;i<3000;i++)
            mas[i]="";
        char *pch;
        int j=0;

       // std::cout<<"ClimatCorrel:\n";
        ReadClimatCorrel(mas);
        int iz=0;
        int im=0;
        int is1=0;
        int is2=0;
        bool iYM=false;
        for (int i=0;i<3000;i++){
          if (mas[i]=="")
             break;

          pch=strtok(mas[i],"\t\r");
                  while(pch!=NULL){
                     std::replace(pch,pch+strlen(pch),',','.');
                     switch (iYM){
                     case false:{
                         Climat::ClimatCorrel[iz][im][is1][is2].CorrZ=atof(pch);
                         break;
                     }
                     case true:{
                         Climat::ClimatCorrel[iz][im][is1][is2].CorrM=atof(pch);
                         break;
                     }
                     default:
                         break;
                    }
                   pch = strtok (NULL,"\t\r");
                   is2++;
                   if (is2>11)
                       is2=0;
                  }
                  iYM=!iYM;
                  if (iYM==0) is1++;
                  if (is1>10){
                      is1=0;
                      im+=1;
                  }
                  if (im>11){
                      im=0;
                      iz++;
                  }
                  if (iz>10){
                      iz=0;
                  }

        }
       return true;
     }
    catch(...)
    {
     return false;
    }
 }

//Считывает файл climatsi
void Climat::ReadClimatSi(char *mas[])
{
    char buf[1000];
    ifstream f("d:\\Luba\\2\\Plow\\debug\\climatsi.txt", ios::in /*| ios::binary*/);
   if (!f)
   {
    cout<<"Error file climatsi.txt open!";
    return ;
   }
   int i=0;
   while (!f.eof())
   {
    f.getline(buf,/*81*/100,'\r\n');
    mas[i]=new char[strlen(buf)];
    strcpy(mas[i],buf);
    i++;
   // system("pause");
   }
   f.close();


    /*std::ifstream ifs("d:\\Luba\\Tasks\\Zemledelie\\build-Plow-Desktop_Qt_5_3_MinGW_32bit-Release\\release\\climatsi.txt", ios::in | ios::binary);  //Теперь открываем
       if(!ifs.is_open()){std::cerr<<"File not found";
           return ;} //если файл уже успели удалить, то exit
        int i = 0;
        std::string s;
        while(getline(ifs,s)){      //Читаем слова где есть кавычки
                //std::cout<<s<<"\n";
                mas[i]=s;
                i+=1;
        }
        ifs.close(); //Закрываем файл*/
}


//Считывает файл climatpr
void Climat::ReadClimatPr(char *mas[])
{
    char buf[1000];
    ifstream f("d:\\Luba\\2\\Plow\\debug\\climatpr.txt", ios::in );
   if (!f)
   {
    cout<<"Error file climatpr.txt open!";
    return ;
   }
   int i=0;
   while (!f.eof())
   {
    f.getline(buf,81,'\r\n');
    mas[i]=new char[strlen(buf)];
    strcpy(mas[i],buf);
    i++;
   }
   f.close();
}

//Считывает файл climatcorrel
void Climat::ReadClimatCorrel(char *mas[])
{
   try{
    char buf[/*3000*/1000];
    ifstream f("d:\\Luba\\2\\Plow\\debug\\clkorrel.txt", ios::in );
   if (!f)
   {
    cout<<"Error file clkorrel.txt open!";
    return ;
   }
   int i=0;
 while (!f.eof())
   //for (int i=0;i<2000;i++)
   {
    f.getline(buf,81,'\r\n');
    mas[i]=new char[strlen(buf)];
    strcpy(mas[i],buf);
    i++;
   }
 f.close();
    }
    catch(...){
       cout<<"Crush ReadClimatCorrel";
    }
}


