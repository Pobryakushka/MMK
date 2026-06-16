#ifndef PARAMSVYL__4_7P_H
#define PARAMSVYL__4_7P_H
#include "OutData/outdata.h"

class ParamsVYl__4_7p
{
public:
    ParamsVYl__4_7p();
    OutData::V GetVYl(double Yl, double Hl, OutData::VLayer V0Hl, OutData::V delV);
    OutData::V GetVYl_f132(double h00, double Yl, double Hl, OutData::VLayer V0Hl, OutData::V delV);
};

#endif // PARAMSVYL__4_7P_H
