#ifndef PARAMSVDVSR_0_200M__3_4P_H
#define PARAMSVDVSR_0_200M__3_4P_H
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"
#include "FunctionsCalc/functionscalc.h"

//Q_ASSERT


class ParamsVdVsr_0_200m__3_4p
{
public:

public:
    ParamsVdVsr_0_200m__3_4p();
    bool Do_3_4(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V075, OutData::VLayer &V0150, OutData::V **VCast);
    bool GetParams_100m(PlowAlgoritm::ProfilePoint *ProfRPV, OutData::V &V100);
    bool GetParams_200m1(int i_m, OutData::V V0hmsr, OutData::V V100d, PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, OutData::VLayer &V0200_1v, Calc_error::Eps &Eps1);
    bool GetParams_0_200_1v(PlowAlgoritm::ProfilePoint *ProfRPV, int i_m, OutData::V V0hmsr, OutData::V V200d, OutData::V &V0200sr_1v);
    bool Get_V_0_200(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,OutData::VLayer &V0200, OutData::VLayer &V0100,
                     OutData::VLayer &V050, OutData::VLayer &V075,OutData::VLayer &V0150, OutData::V **VCast);
    bool Raspred(PlowAlgoritm::ProfilePoint *ProfRPV, double hm, double hj, int hl, int i_m, int i_j, int i_l, OutData::V V100d, OutData::VLayer &V0hm);
    Calc_error::Eps GetEps0200(const ClimatData* cl);
    OutData::V GetV200_f113(double level, double hm, OutData::V V0200, OutData::VLayer V0hm);
    OutData::V GetV0100_f114(OutData::VLayer V0200, OutData::V V100);
    OutData::VLayer GetV050(PlowAlgoritm::ProfilePoint *ProfRPV, OutData::VLayer V0100);
    bool Method0hm(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int n1, OutData::V V100, OutData::V &V0200_1, Calc_error::Eps &Eps1);
    OutData::V GetV0hm_1v(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData* cl,int im, OutData::V V100, FunctionsCalc::Gr* gr, Calc_error::Eps &Epsm);
    OutData::V GetV0hm_f107(PlowAlgoritm::ProfilePoint *ProfRPV,const ClimatData* cl, int i, int j, Calc_error::Eps &Epsm);
    OutData::V GetV0hm(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int i_m, OutData::V V100);
    OutData::V GetV75(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V V50, OutData::V V100);
    OutData::V GetV150(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V V100, OutData::V V200);

public:
   /* PlowAlgoritm::ProfilePoint * ProfRPV;
    const ClimatData* cl;*/
};

#endif // PARAMSVDVSR_0_200M__3_4P_H
