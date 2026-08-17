#ifndef CASTH__1P_H
#define CASTH__1P_H
#include "InData/Constants.h"
#include "OutData/outdata.h"
#include "climatdata.h"

class CastH__1p
{
public:
    //Массив приведенных к высоте расчетной точки значений скорсти ветра из метеобюллетеней
   // static OutData::V **VCast;

public:
    CastH__1p();
    void GetVCast(const ClimatData *cl, OutData::V **VCast);
    void GetMasVBM(OutData::V **VMeteoB);
    OutData::V ProcCast(const ClimatData *cl, int k, int lev, OutData::V **VMeteoB);
    OutData::V ProcCast_1v(const ClimatData* cl, int k, int lev,OutData::V **VMeteoB);
    OutData::V ProcCast_2v(const ClimatData *cl, int k, int lev, OutData::V **VMeteoB);
    OutData::V GetVclh1h2_f4(const ClimatData *cl, double h1, double h2);
    OutData::V GetV0h_f10(double h,int k, OutData::V **VMeteoB);
    OutData::V GetV0h_f12(const ClimatData *cl, double h, int k, OutData::V **VMeteoB);
    OutData::V GetV0h1_f13(const ClimatData *cl, double h1, int k, OutData::V **VMeteoB);
    ClimatData::Pr GetV0h1cl_f14(double h1,ClimatData::Pr V0200cl);
};

#endif // CASTH__1P_H
