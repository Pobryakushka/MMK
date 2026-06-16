#ifndef GETPROFILE__6P_H
#define GETPROFILE__6P_H

#include "OutData/outdata.h"
//#include "InData/constants.h"

class GetProfile__6p
{
public:
    static OutData::ProfileLayer ProfileV[] ;
public:
    GetProfile__6p();
    void Do_6p();
    void GetProfileLayer();
    OutData::Profile GetProfile(OutData::V V);
    void Do_6p_after2p(OutData::V* VsrYl, float *realWind, float *averageWind);
    void GetProfile(OutData::V V, float &Vpr, float &TETA);
};

#endif // GETPROFILE__6P_H
