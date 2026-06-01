#ifndef PARAMSVDVSR_HL__4_1P_H
#define PARAMSVDVSR_HL__4_1P_H
#include "OutData/outdata.h"
#include "InData/InData.h"
#include "climatdata.h"
#include "FunctionsCalc/functionscalc.h"
#include "FunctionsCalc/calc_error.h"

class ParamsVdVsr_Hl__4_1p
{
public:
    //Признак невозможности расчетов параметров действительного
    //ветра на верхней границе стандартного слоя Yl
    static int IPRL;
    struct L{
      double hi;
      OutData::V Vi;
    };
public:
    ParamsVdVsr_Hl__4_1p();
    bool Do_4_1p(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int l, double &Hl, OutData::VLayer &V0Hl);
    OutData::VLayer GetV0Hlsr(PlowAlgoritm::ProfilePoint *ProfRPV, double Hl, int l);
    OutData::VLayer GetV0Hlsr_an(PlowAlgoritm::ProfilePoint *ProfRPV, double Hl);
    OutData::VLayer GetV0Hlsr_l2(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData *cl, double Hl);
    OutData::VLayer GetV0Hlsr_an2(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, OutData::V V0h1);
    int GetHl(PlowAlgoritm::ProfilePoint *ProfRPV, double Yl);
    OutData::V GetV0Hl(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int i_m);
    OutData::V GetV0hm_1v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl,int im, FunctionsCalc::Gr* gr, Calc_error::Eps &Epsm);
    bool Method0Hl(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int n1, OutData::V &V0200_1, Calc_error::Eps &Eps1);
    OutData::VLayer GetV0Hlsr_l2_niz(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData *cl, double Hl);
    OutData::VLayer GetV0Hlsr_an3(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, OutData::V V0h1);

public:
    PlowAlgoritm::ProfilePoint * ProfRPV;
    const ClimatData* cl;
};

#endif // PARAMSVDVSR_HL__4_1P_H
