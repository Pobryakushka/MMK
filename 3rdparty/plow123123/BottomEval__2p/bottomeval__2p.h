#ifndef BOTTOMEVAL__2P_H
#define BOTTOMEVAL__2P_H
//#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"
#include "FunctionsCalc/mathfunc.h"

class BottomEval__2p
{
public:
    BottomEval__2p();
    void GetVsr__2p(const ClimatData *cl, OutData::V *VsrYl, OutData::V **VCast);
    void GetVsr__2_2p(const ClimatData *cl, OutData::V *VsrYl);
    int GetNumMB__2_3p(double V0200sr, double TETA0200sr, double *deltau, OutData::V **VCast);
    double GetdelVdop(double deltau);
    double GetdelV(double V0, double TETA0, double Vj, double TETAj);
    OutData::V GetV0200__2_4p(const ClimatData *cl, OutData::V V02001, int numMB, double deltau);
    void GetPZM(double EpsZM1, double EpsZM2, double *P);
    int GetNumMB_H__2_5p(OutData::V V0200corr, double *deltau,OutData::V **VCast);
    void GetVsr_H__2_6(const ClimatData *cl, int numMB_H, double deltauj, OutData::V **VCast, OutData::V *VsrYl);
    OutData::V delV0Yl0200_t_1v(const ClimatData *cl, int l, OutData::V V0200, Calc_error::Eps &Eps1);
    Calc_error::Eps Eps_1v(const ClimatData *cl, double Yl, OutData::V V0200);
    OutData::V delV0Yl0200_t_2v(const ClimatData *cl, int l, int numMB_H, double deltauj, OutData::V **VCast, OutData::V _V0200,Calc_error::Eps &Eps2);
    Calc_error::Eps Eps_2v(const ClimatData *cl, double Yl, double h, double deltauj, OutData::V _V0200, OutData::V V0200j);
    OutData::V GetDelV0Yl0200_t(const ClimatData *cl, int l, int numMB_H, double deltauj, OutData::V V0200, OutData::V **VCast);
    int GetM(MathFunc::Rank *Rankmas,int k,int ind);
    Calc_error::Eps GetEps_f28(double deltau);
    OutData::V GetV0200_f30(OutData::V V02001, OutData::V V02002, Calc_error::Eps Eps1,Calc_error::Eps Eps2);
};

#endif // BOTTOMEVAL__2P_H
