#ifndef WINDSHEAR_H
#define WINDSHEAR_H
#include "InData/InData.h"

class WindShear
{
public:
    enum MessErrors{RESULT_OK, WINDSHEAR_NOT_CONFIG_ALG};
    PlowAlgoritm::ProfilePoint * ProfRPV;
    int kwsh;//кол-во уровней для расчета сдвига ветра
public:
    WindShear(float *windShear);
    WindShear::MessErrors GetWindShear(float *windShear);
    void setProfRpv(PlowAlgoritm::ProfilePoint* prof, int hCount);

    ~WindShear();
private:
    float GetDelV(float V1, float alf1, float V2, float alf2);
    float GetDelAlf(float V1, float alf1, float V2, float alf2, float delV);
    int sign(float gam);
};

#endif // WINDSHEAR_H
