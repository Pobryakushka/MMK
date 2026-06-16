#ifndef PARAMSMATRIX__4_4P_H
#define PARAMSMATRIX__4_4P_H
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_error.h"
#include "InData/Constants.h"

class ParamsMatrix__4_4p
{
public:
//    static constexpr double epsNoMod=/*100*/1000;//!!!
    static constexpr double epsNoMod=/*100*//*1000*/1e32;//!!!
    struct M{
        double delV/*=Constants::UNDEF_V*/;
        double Eps/*=epsNoMod*/;//чтобы несуществующие модели были на последнем месте
        int nm/*=-1*/;//номер места модели
        M(){
          delV=Constants::UNDEF_V;
          Eps=epsNoMod;
          nm=-1;
        }
    };
    static const int numMod;
    struct vectMod{
        double epsMod;
        int iMod;
    };

public:
    ParamsMatrix__4_4p();
    void GetFormMatrix(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int l, double Yl, double Hl, OutData::VLayer V0Hl, OutData::V **VCast, int &NN, int iHl, int ihn, M **MZ, M **MM, int &kmod);
    int GetNl_4_4_1p(PlowAlgoritm::ProfilePoint *ProfRPV, double Yl);
    void Get4lModel_4_4_2p(PlowAlgoritm::ProfilePoint* ProfRPV, double Yl,int Nl, bool &m4, bool &m3,int &i1, int &i2, int &i3);
    double hsh(int Nl, double y, double hmas[],int &imin);
    double GetSG(double y1, double Yl, double hsh1,double hsh2,double hsh3);
    void Do4lModel_4_4_4p(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int i1, int i2, int i3, int l, double Yl, double Hl, OutData::VLayer V0Hl, int &NN, int iHl, int ihn, OutData::V **VCast, M **MZ, M **MM,
                          double &HH, OutData::VLayer &V0HH);
    bool Check3lModel_4_4_3p(PlowAlgoritm::ProfilePoint *ProfRPV, int Nl, double Yl, int &j1, int &j2);
    double GetSG_443(double y1, double Yl, double hsh1,double hsh2);
    OutData::V GetV0hsh1(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, double hsh1, OutData::V V0hsh1d);
    OutData::V GetV0hsh1_4_4_7(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData *cl, double hsh1, OutData::V V0hsh1d);
    OutData::VLayer GetV0Hlsr_f136(PlowAlgoritm::ProfilePoint *ProfRPV, double hsh1, double Hl, OutData::V V0hsh1);
    ClimatData::Pr GetV0hshcl(const ClimatData* cl, double hsh);
    OutData::V GetdelVrcl(OutData::V V0h2, OutData::V V0h1, OutData::V V0h2cl, OutData::V V0h1cl);
    OutData::V GetdelV0Yl0hsh3_1v(const ClimatData *cl, double Yl, double hsh3, double delh1, double delh2, double delh3, double delh4,
                                  OutData::V delV0Yl0hsh3cl, OutData::V A, OutData::V B, Calc_error::Eps &Eps1);
    Calc_error::Eps GetEps(const ClimatData *cl, double Yl, double hsh3, OutData::V delV0Yl0hsh3, OutData::V delV0Yl0hsh3cl);
    OutData::V GetdelV0Yl0hsh3_2v(const ClimatData *cl, double Yl, double hsh3, double delh1, double delh2, double delh3, double delh4,
                                  OutData::V delV0Yl0hsh3cl, OutData::V delVrcl32, Calc_error::Eps &Eps2);
    OutData::V GetdelV0Yl0hsh3_3v(const ClimatData *cl, double Yl, double hsh3, double delh1, double delh2, double delh3, double delh4,
                                  OutData::V delV0Yl0hsh3cl, OutData::V A, OutData::V B1, Calc_error::Eps &Eps3);
    void GetKoeff(double delh1,double delh2,double delh3,double delh4,OutData::V delVrcl21,OutData::V delVrcl32,
                                      OutData::V &A,OutData::V &B, OutData::V &B1,OutData::V &A2,OutData::V &B2);
    OutData::V GetdelV0Yl0hsh3_4v(const ClimatData *cl, double Yl, double hsh3, double delh1, double delh2, double delh3, double delh4,
                                                      OutData::V delV0Yl0hsh3cl, OutData::V A2, OutData::V B2, Calc_error::Eps &Eps4);
    Calc_error::Eps GetEps_f146(const ClimatData *cl, double Yl, double hsh3, OutData::V V0hsh3, OutData::V V0hsh3cl);
    OutData::V GetdelV0Yl0hsh3_5v(const ClimatData *cl, double Yl, double hsh3, OutData::V delV0Yl0hsh3cl, OutData::V V0hsh3, OutData::V V0hsh3cl, Calc_error::Eps &Eps5);
    OutData::V GetdelV0Yl0hsh3_6v(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int im, double Yl, double Hl, double h00,
                                 OutData::VLayer V0Hl, OutData::V V0hsh3sr, Calc_error::Eps &Eps6, double &delhy);
    OutData::V GetdelV0Yl0hsh3_7v(const ClimatData *cl, double Yl, double h, OutData::V V0Hlsr, OutData::V **VCast,
                                                      Calc_error::Eps &Eps7);
    int Getnl(PlowAlgoritm::ProfilePoint *ProfRPV, double Hl, int &iHl, int &ihn);
    void GetVarMod(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int i1, int i2, int i3, int l, double Yl, double Hl, OutData::VLayer V0Hl, double delhyIn, OutData::V **VCast, double &delhyOut,
                   OutData::VLayer &V0hsh3 , OutData::V *delV0Yl0hsh3, Calc_error::Eps *eps);
    OutData::V GetdelV0hi0h2sh3(OutData::V V0hisr, OutData::V V0h2sh3);
    void FormMatrix_4_4_6p(M **MZ,M **MM, int inum, int jnum);
    void RankingM(M **MX,int jnum, int inum);
    OutData::V GetdelV0Yl0hsh2_1v(const ClimatData *cl, double Yl, double hsh2, double delh1, double delh2, double delh3,
                                                      OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1);

    OutData::V GetdelV0Yl0hsh2_2v(const ClimatData *cl, double Yl, double hsh2, double delh1, double delh2, double delh3,
                                                      OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1);
    OutData::V GetdelV0Yl0hsh2_3v(const ClimatData* cl,double Yl, double hsh2, double delh1, double delh2,double delh3,
                                  OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1);
    OutData::V GetdelV0Yl0hsh2_4v(const ClimatData *cl, double Yl, double hsh2, double delh2, double delh3,
                                                      OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1);
    void GetVarMod_443(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int j1, int j2, int l, double Yl, double Hl, OutData::VLayer V0Hl,
                       double delhyIn, OutData::V** VCast, double &delhyOut, OutData::VLayer &V0hsh3, OutData::V *delV0Yl0hsh3, Calc_error::Eps *eps);
    void Do4lModel_4_4_7p(PlowAlgoritm::ProfilePoint *ProvRPV, const ClimatData *cl, int j1, int j2, int l, double Yl, double Hl, OutData::VLayer V0Hl, int &NN, int iHl, int ihn, OutData::V **VCast, M **MZ, M **MM,
                            double &HH, OutData::VLayer &V0HH);
    void GetDel_4_4_5p(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int l,double Yl,double Hl,
                       OutData::V V0Hlsr,OutData::V **VCast,int &NN, M **MZ, M **MM);
    int GetNN(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, int &iHl, int &ihn);
    OutData::V GetdelV_3v(const ClimatData *cl, int l, double Hl, OutData::V V0Hlsr, OutData::V **VCast, Calc_error::Eps &Eps3);
    int GetNumMB_H__2_5p(const ClimatData *cl, double Hl, int l, OutData::V V0Hlsr, OutData::V **VCast,
                                           OutData::V &delV_3v, Calc_error::Eps &Eps3);
    OutData::V GetdelV_7v(const ClimatData *cl, double Yl, double Hl, OutData::V V0Hlsr, OutData::V **VCast, Calc_error::Eps &Eps3);
    int GetNumMB_H7v__2_5p(const ClimatData *cl, double Yl, double Hl, OutData::V V0Hlsr, OutData::V **VCast,
                                           OutData::V &delV_3v, Calc_error::Eps &Eps3);
    OutData::V GetV0h1sh(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int ihsh1, int ihsh2);
};

#endif // PARAMSMATRIX__4_4P_H
