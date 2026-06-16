#ifndef PARAMSDELV__4_6P_H
#define PARAMSDELV__4_6P_H
#include "OutData/outdata.h"
#include "ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.h"

class ParamsdelV__4_6p
{
public:
    ParamsdelV__4_6p();
    OutData::V GetdelV(int kmod,bool after_4_3,int NN, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V GetdelV1_f129();
    OutData::V GetdelV2_f130();
    OutData::V GetdelV12_f130a();
    bool GetPi1i2i3(int i1, int i2, int i3);
    OutData::V GetdelV3_f131();
    OutData::V delVafter_4_3(int kmod);
    OutData::V delVafter_4_5(int kmod, int NN,ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V delVkmod3(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V delVkmod4(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V delVkmod5(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V delVkmod6(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
    OutData::V delVkmod7(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
};

#endif // PARAMSDELV__4_6P_H
