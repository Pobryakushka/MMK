#include "paramsvyl__4_7p.h"
#include "InData/InData.h"
#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"

ParamsVYl__4_7p::ParamsVYl__4_7p()
{

}

OutData::V ParamsVYl__4_7p::GetVYl(double Yl, double Hl, OutData::VLayer V0Hl, OutData::V delV){
    OutData::V VYl;
    if (ParamsVdVsr_Hl__4_1p::IPRL==0){
        ParamsdelV__4_3p::IPRHH=0;
        return VYl;
    }
    double h00=0;
    if (InData::bottomLayer)
        h00=InData::h0;
    VYl=GetVYl_f132(h00, Yl, Hl, V0Hl, delV);//ф.132
    return VYl;
}

OutData::V ParamsVYl__4_7p::GetVYl_f132(double h00, double Yl, double Hl, OutData::VLayer V0Hl, OutData::V delV){
  OutData::V VYl;
  VYl.VZ=(2*delV.VZ*(Yl-h00)+2*V0Hl.Vsr.VZ*(Yl-Hl)-V0Hl.Vd.VZ*(Yl-Hl))/(Yl-Hl);//132
  VYl.VM=(2*delV.VM*(Yl-h00)+2*V0Hl.Vsr.VM*(Yl-Hl)-V0Hl.Vd.VM*(Yl-Hl))/(Yl-Hl);
  return VYl;
}



