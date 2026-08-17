#ifndef PARAMSVDVSR_0_200M__3P_H
#define PARAMSVDVSR_0_200M__3P_H
#include "OutData/outdata.h"
#include "InData/InData.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"

class ParamsVdVsr_0_200m__3p
{
public:
   static OutData::VLayer V0200_3p, V0100_3p, V050_3p, V025_3p, V075_3p, V0150_3p;
public:
    ParamsVdVsr_0_200m__3p();
    bool Do_3p(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, OutData::V **VCast);
};

#endif // PARAMSVDVSR_0_200M__3P_H
