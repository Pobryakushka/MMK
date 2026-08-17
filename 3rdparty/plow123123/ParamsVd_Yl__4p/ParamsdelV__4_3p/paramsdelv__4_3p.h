#ifndef PARAMSDELV__4_3P_H
#define PARAMSDELV__4_3P_H
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"

class ParamsdelV__4_3p
{
public:
    //Структура способв расчета
   struct Mode{
        OutData::V delV0Yl0Hl;
        Calc_error::Eps Eps;
        Calc_error::P P;
    };
    //Массив способов расчета
    static Mode ComplMode[3];
    static int IPRHH;
public:
    ParamsdelV__4_3p();
    bool GetdelV(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int l, double Yl, double Hl, OutData::V V0Hlsr, OutData::V **VCast, int &kmod);
    OutData::V GetdelV_1v(const ClimatData *cl, double Yl, double Hl, OutData::V V0Hlsr, Calc_error::Eps &Eps1);
    Calc_error::Eps GetEps1(const ClimatData *cl, double Yl, double Hl, OutData::V V0Hlsr, ClimatData::Pr V0Hlcl);
    OutData::V GetdelV_2v(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int l, double Yl, double Hl, Calc_error::Eps &Eps2);
    OutData::V GetdelV2_f120(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, double Yl, double Hl, int m, OutData::VLayer V0Hl);
    OutData::V GetdelV2_f122(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, double Yl, double Hl, double hm, OutData::VLayer V0hm, Calc_error::Eps &Eps2);
    OutData::V GetdelV_3v(const ClimatData *cl, double Yl, double Hl, OutData::V V0Hlsr, OutData::V **VCast, Calc_error::Eps &Eps3);
    int GetNumMB_H__2_5p(const ClimatData* cl, double Yl,double Hl/*,int l*/,OutData::V V0Hlsr,OutData::V **VCast,
                         OutData::V &delV_3v, Calc_error::Eps &Eps3);
    int Getkmod();

public:
  //  PlowAlgoritm::ProfilePoint * ProfRPV;
    const ClimatData* cl;
};

#endif // PARAMSDELV__4_3P_H
