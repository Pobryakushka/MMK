#ifndef PARAMSVD_YL__4_2P_H
#define PARAMSVD_YL__4_2P_H
#include "OutData/outdata.h"
#include "InData/InData.h"

class ParamsVd_Yl__4_2p
{
public:
    ParamsVd_Yl__4_2p();
    OutData::V GetVdYl(PlowAlgoritm::ProfilePoint *ProfRPV, int m, int mpl1, double Yl);
};

#endif // PARAMSVD_YL__4_2P_H
