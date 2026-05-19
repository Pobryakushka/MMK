#ifndef OUTDATA_H
#define OUTDATA_H
#include "InData/Constants.h"

using namespace PlowAlgoritm;

class OutData
{
public:
    struct V{
       double VZ/*=Constants::UNDEF_V*/;
       double VM/*=Constants::UNDEF_V*/;
       V(){
          VZ=Constants::UNDEF_V;
          VM=Constants::UNDEF_V;
       }
   };
   //Структура параметров ветра в слое
    struct VLayer{
        V Vd;
        V Vsr;
    };

    struct Profile{
       double V;
       double TETA;
       Profile(){
           V=Constants::UNDEF_V;
           TETA=Constants::UNDEF_TETA;
       }
   };
   //Структура вертикального профиля
   //скорости и направления ветра
    struct ProfileLayer{
       Profile Pd;
       Profile Psr;
       double layer;
   };

public:
    OutData();
};

#endif // OUTDATA_H
