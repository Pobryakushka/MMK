#ifndef PARAMSVDVSR_0_200M__3_2P_H
#define PARAMSVDVSR_0_200M__3_2P_H
#include "FunctionsCalc/calc_error.h"
#include "FunctionsCalc/functionscalc.h"

class ParamsVdVsr_0_200m__3_2p
{
public:
    ParamsVdVsr_0_200m__3_2p();
    bool Do_3_2(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::V **VCast);
    bool Get_Vsr_0_200(OutData::VLayer &VZ0200, OutData::VLayer &VZ0100, OutData::V **VCast);
    int GetNumMB(OutData::V **VCast, double *deltau , OutData::V V0200_1);
    double GetdelV_f82(double Vj, double TETAj,OutData::V V0200_1);
    OutData::V GetV0200pZ(OutData::V V0200_1,Calc_error::Eps Eps1,OutData::V **VCast);
    bool Method0h1(int n1, OutData::V &V0200_1,Calc_error::Eps &Eps1);
    OutData::V GetV0h1(int i, int j, Calc_error::Eps &Eps0h1);
    OutData::V GetV0200_1(FunctionsCalc::Gr* gr, Calc_error::Eps &Eps1);
    OutData::V GetV0200_1_2k(Calc_error::Eps &Eps1);

public:
    PlowAlgoritm::ProfilePoint * ProfRPV;
    const ClimatData* cl;

};

#endif // PARAMSVDVSR_0_200M__3_2P_H
