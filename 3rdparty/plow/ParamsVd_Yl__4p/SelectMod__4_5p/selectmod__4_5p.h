#ifndef SELECTMOD__4_5P_H
#define SELECTMOD__4_5P_H
#include "ParamsVd_Yl__4p/ParamsMatrix__4_4p/paramsmatrix__4_4p.h"

class SelectMod__4_5p
{
public:
    struct SUM{
        double sum;
        int ind;
    };

public:
    SelectMod__4_5p();
    void Get3Mod(int NN, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM);
};

#endif // SELECTMOD__4_5P_H
