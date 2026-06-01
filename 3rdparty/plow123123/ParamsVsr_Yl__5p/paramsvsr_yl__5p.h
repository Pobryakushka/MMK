#ifndef PARAMSVSR_YL__5P_H
#define PARAMSVSR_YL__5P_H
#include "OutData/outdata.h"
#include "InData/Constants.h"
#include "InData/InData.h"
#include "InData/indataclimat.h"


class ParamsVsr_Yl__5p
{
public:
    static OutData::VLayer V0Yl[] ;
public:
    ParamsVsr_Yl__5p();
    void Do_5p(PlowAlgoritm::ProfilePoint *ProfRPV, int l, double Hl, OutData::VLayer V0Hl, OutData::V VYl,
                                 OutData::VLayer V0HH, double HH, OutData::V delV0Yl0Hl);
    OutData::V GetV0Yl_166f(double h00, double Yl, double Hl, OutData::VLayer V0Hl, OutData::V VYl);
    OutData::V GetV0Yl_167f(OutData::V V0Hlsr, OutData::V delV0Yl0Hl);
    OutData::V GetV0Yl_168f(double h00, double Yl, double HH, OutData::VLayer V0HH, OutData::V VYl);
};

#endif // PARAMSVSR_YL__5P_H


