#include "fileprocessing.h"
//#include <iostream>
//#include <fstream>
#include <string>
#include <InData/InData.h>


#include "fileprocessing.h"


using namespace std;

FileProcessing::FileProcessing()
{

}

///Считывает файл параметров вертикального профиля ветра
bool FileProcessing::ReadRPVprof(int k, char *mas[],char* fileName)
{
    char buf[1000];

    //ifstream f("d:\\Luba\\2\\Plow\\debug\\profRPV_m.txt", ios::in | ios::binary);
    ifstream f(fileName, ios::in | ios::binary);
   if (!f)
   {
    cout<<"Error file profRPV_m.txt open!";
    return false;
   }
  // while (!f.eof())
   for (int i=0;i<k;i++)
   {
    f.getline(buf,81,'\n');
    mas[i]=new char[strlen(buf)+1];
    strcpy(mas[i],buf);
   }
   return true;
}
