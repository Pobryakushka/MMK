#ifndef PARAMSVD_YL__4P_H
#define PARAMSVD_YL__4P_H
#include "OutData/outdata.h"
#include "InData/indataclimat.h"

class ParamsVd_Yl__4p
{
public:
static double  Hlmas[] ;
static OutData::VLayer V0Hl[] ;
static int INT;
static double HH;
static OutData::VLayer V0HH;

public:
    ParamsVd_Yl__4p();
    bool Do_4p(PlowAlgoritm::ProfilePoint *ProfRPV, const ClimatData *cl, int l, OutData::V &Vd_Yl, OutData::V **VCast, OutData::V &delV0Yl0Hl);
};

#endif // PARAMSVD_YL__4P_H
