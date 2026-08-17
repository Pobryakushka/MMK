#ifndef PARAMSVDVSR_0_200M__3_3P_H
#define PARAMSVDVSR_0_200M__3_3P_H
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"

class ParamsVdVsr_0_200m__3_3p
{
public:
    struct L{
      double hi;
      OutData::V Vi;
    };

public:
    ParamsVdVsr_0_200m__3_3p();
    bool Do_3_3(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150, OutData::V **VCast);
    bool Get_V_0_200(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, OutData::VLayer &VZ0200, OutData::VLayer &VZ0100, OutData::VLayer &VZ050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150, OutData::V **VCast);
    bool Get_V_0_200_1v(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int i, int ipl1,  OutData::V V0, OutData::VLayer &V0200_1, int &M, Calc_error::Eps &Eps1);
    bool GetV0layer(PlowAlgoritm::ProfilePoint *ProfRPV, double layer, OutData::V V0, OutData::V Vlayer, int &M, OutData::V &V0layer);
    bool GetV0200_1s(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData *cl, int i, int ipl1, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150, OutData::V **VCast);
    bool GetV0200_2s(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150, OutData::V **VCast);
    bool GetV0200_1v_mpl1(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int m, OutData::V Vhm, OutData::V V0hm, OutData::V &V0200_1, Calc_error::Eps &Eps1);
    OutData::V GetV0200_f101(double hm, double hmpl1, OutData::V V0hm,OutData::V V0hmpl1);
   // bool GetV0200_2v_mpl1(OutData::V &V0200_2, Calc_error::Eps &Eps2);
    OutData::V GetV0200_f103(const ClimatData* cl, double hmpl1, OutData::V V0hmpl1);
    OutData::V GetV0200_f100(PlowAlgoritm::ProfilePoint* ProfRPV, int m, OutData::V Vhm,
                             OutData::V V0hm);
};

#endif // PARAMSVDVSR_0_200M__3_3P_H
