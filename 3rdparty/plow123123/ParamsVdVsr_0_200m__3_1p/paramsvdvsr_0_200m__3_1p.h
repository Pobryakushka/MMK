#ifndef PARAMSVDVSR_0_200M__3_1P_H
#define PARAMSVDVSR_0_200M__3_1P_H
#include <list>
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"
#include "FunctionsCalc/functionscalc.h"


class ParamsVdVsr_0_200m__3_1p
{
public:
    //Структура способв расчета
    struct Mode{
        OutData::V V0200;
        Calc_error::Eps Eps;
        Calc_error::P P;
    };
    //Массив способов расчета
   static Mode ComplMode[/*4*/];
   //Структура для ранжирования результатов расчета
struct ei{
    double eps;
    int ind;
};
    struct EpsInd{
       ei Z;
       ei M;
    };
    struct IPR{
        int Z;
        int M;
    };


public:
    ParamsVdVsr_0_200m__3_1p();
    bool Do_3_1(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V150, OutData::V **VCast);
    bool Get_Vsr_0_200(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, OutData::VLayer &VZ0200, OutData::VLayer &VZ0100, OutData::VLayer &VZ050, OutData::V **VCast);
    bool Get_Vsr_0_200_1v(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl);
    bool Get_Vsr_0_200_2v(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl);
    bool Get_Eps_2v(const ClimatData *cl, double level, double h, Calc_error::Eps Eps0h, OutData::V V0hn1, Calc_error::Eps &Eps);
    bool Get_Vsr_0_200_3v(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl);
    bool Get_Eps_3v(const ClimatData *cl, double level, double h, Calc_error::Eps &Eps);
    bool Get_Vsr_0_200_4v(OutData::V **VCast);
    bool GetP(char* comb);
    bool GetP13();
    bool GetPi1i2i3(int i1, int i2, int i3);
    void GetEpsInd(int i1, int i2, int i3,EpsInd *EpsIndZM);
    bool GetPi1i2i3();
    //EpsInd* GetEpsInd(int i1, int i2, int i3, int i4);
    void GetEpsInd(int i1, int i2, int i3, int i4, EpsInd *EpsIndZM);
    void GetEpsInd(EpsInd *EpsIndZM);
    bool GetVZ0200(OutData::V &VZ0200);
    IPR GetIPR(FunctionsCalc::delV delVt, FunctionsCalc::delV delVtsh);
    void GetP72(ParamsVdVsr_0_200m__3_1p::IPR IPR, Calc_error::P *P);
    void GetP75(ParamsVdVsr_0_200m__3_1p::IPR IPR,Calc_error::P *P);
    OutData::V GetV0200_f43(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, double h1, double h2, double h3, Calc_error::Eps &Eps0h1);
    ClimatData::Pr GetV0200_f43_43(const ClimatData *cl, double h1, OutData::V V0h1);
    int GetNumMB(OutData::V **VCast , double *deltau);
    double GetdelV_f49(double Vj, double TETAj);
    void GetVTETA0200(double &V0200_t, double &TETA0200_t);
    bool GetPi1i2i3i4();

    void GetMasVZM(PlowAlgoritm::ProfilePoint *ProfRPV, OutData::V *VZM);
    bool Method0hn1(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData *cl, int n1, OutData::V &V0200_2, Calc_error::Eps &Eps2);
    list<int> FormMK(int n1,PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V *VZM, FunctionsCalc::Gr* _gr ,int &nk);
    OutData::V GetV0hn1_1v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                    double hi, double hj, double hn1);
    OutData::V GetV0hn1_2v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                    double hi, double hj, double hn1);
    OutData::V GetV0hn1_3v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                    double hi, double hj);

public:
    /*PlowAlgoritm::ProfilePoint * ProfRPV;
    const ClimatData* cl;*/
};



#endif // PARAMSVDVSR_0_200M__3_1P_H
