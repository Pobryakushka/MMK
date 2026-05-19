#ifndef PROFILE_H
#define PROFILE_H
//#include "OutData/outdata.h"
//#include "InData/InData.h"
//#include "climatdata.h"
//#include "InData/indataclimat.h"
#include <vector>
#include "../InData/Constants.h"

//class PlowAlgoritm::ProfilePoint;
class MBulletin;
class ClimatData;

class ProfileReal
{
public:
    enum MessErrors{RESULT_OK, PROFILE_NOT_CONFIG_PROF_RPV, PROFILE_CLIMAT_NOT_READY, PROFILE_NOT_CONFIG_HEIGHT};

    ProfileReal(/*float *realWind, float *averageWind*/);
    ProfileReal::MessErrors GetProfile(float *realWind, float *averageWind);
    void setProfRpv(PlowAlgoritm::ProfilePoint *prof , int hCount);
    void setMeteoMid(const MBulletin* p, int size, bool prevzond);
    void setGroundWind(float speed, float azimut, float height);
//    void setBLH(double B, double L, double H);
    void setHeight(double H);
    void setDateTime(int GD0, int MS0, int DN0, double CH0);
    void setz0(float z0);
    void setClimatData(const ClimatData* cl) {
        this->cl = cl;
    }
    void End(void *dMasToDel, int numRow);
    ~ProfileReal();

public:
    PlowAlgoritm::ProfilePoint *ProfRPV = nullptr;
    const ClimatData *cl = nullptr;
private:
    void SetInitial();
};

#endif // PROFILE_H
