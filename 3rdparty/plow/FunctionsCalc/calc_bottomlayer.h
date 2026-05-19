#ifndef CALC_BOTTOMLAYER_H
#define CALC_BOTTOMLAYER_H

#include "InData/InData.h"

class Calc_bottomLayer
{
public:
    Calc_bottomLayer();

    //Оценка параметров среднего ветра   в слое 0÷200 м по параметрам приземного ветра
    //ф. 20, блок 2.1
    static bool Get_V0_0_200(double &V0_0_200, double &TETA_0_200){
        try{
            if (InData::V0_tau0<=8/*м/с*/)
                V0_0_200=InData::V0_tau0+3/*м/с*/;
            else
                V0_0_200=InData::V0_tau0+5/*м/с*/;
            TETA_0_200=InData::TETA0_tau0+6/*градус*/;
            if (TETA_0_200>360)
                 TETA_0_200=TETA_0_200-360;
            return true;
        }
        catch(...){
            return false;
        }
    }
};

#endif // CALC_BOTTOMLAYER_H
