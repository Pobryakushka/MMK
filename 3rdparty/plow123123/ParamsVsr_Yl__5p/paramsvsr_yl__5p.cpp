#include "paramsvsr_yl__5p.h"
#include "ParamsVd_Yl__4p/paramsvd_yl__4p.h"
#include "InData/indataclimat.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"
#include "ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"

OutData::VLayer ParamsVsr_Yl__5p::V0Yl[Constants::numStL];

ParamsVYl__4_7p paramsVYl__4_7p1;
ParamsVdVsr_Hl__4_1p paramsVdVsr_Hl__4_1p2;

ParamsVsr_Yl__5p::ParamsVsr_Yl__5p()
{

}
void ParamsVsr_Yl__5p::Do_5p(PlowAlgoritm::ProfilePoint* ProfRPV, int l, double Hl, OutData::VLayer V0Hl, OutData::V VYl,
                             OutData::VLayer V0HH, double HH, OutData::V delV0Yl0Hl){
   // Hl=0;
    V0Yl[l-1].Vd.VZ=VYl.VZ;
    V0Yl[l-1].Vd.VM=VYl.VM;
    double Yl=Constants::StandartLayer[l-1];
    double h00=0;
    if (InData::bottomLayer)
        h00=InData::h0;
    if (ParamsVd_Yl__4p::INT==1){
      if((V0Hl.Vd.VZ!=Constants::UNDEF_V)&&(V0Hl.Vd.VM!=Constants::UNDEF_V))
          V0Yl[l-1].Vsr=GetV0Yl_166f(h00, Yl, Hl, V0Hl, V0Yl[l-1].Vd);
      else{
          //комбинации
          int ish1;
          FunctionsCalc::Poisk_hm(ProfRPV,Yl,ish1);
          //ан. 116, ф.135а
           V0Yl[l-1].Vsr=paramsVdVsr_Hl__4_1p2.GetV0Hlsr_an(ProfRPV,Yl).Vsr;//ан.116
      }
      return;
    }
    if (ParamsVd_Yl__4p::INT==0){
        if(ParamsdelV__4_3p::IPRHH==0){
            V0Yl[l-1].Vsr=GetV0Yl_167f(V0Hl.Vsr, delV0Yl0Hl);//ф.167
        }
        if(ParamsdelV__4_3p::IPRHH==1){
            if(V0HH.Vd.VM == -999) {
                V0Yl[l-1].Vsr.VM = Constants::UNDEF_V;
                V0Yl[l-1].Vsr.VZ = Constants::UNDEF_V;
                V0Yl[l-1].Vd.VM = Constants::UNDEF_V;
                V0Yl[l-1].Vd.VZ = Constants::UNDEF_V;
            } else {
                //ф.168
                V0Yl[l-1].Vsr=GetV0Yl_168f(h00, Yl, HH, V0HH, VYl);
                //Корректировка параметров действ. ветра
                OutData::V delV0Yl0Hl_t;
                delV0Yl0Hl_t.VZ=V0Yl[l-1].Vsr.VZ-V0Hl.Vsr.VZ;//ф.169
                delV0Yl0Hl_t.VM=V0Yl[l-1].Vsr.VM-V0Hl.Vsr.VM;
                V0Yl[l-1].Vd=paramsVYl__4_7p1.GetVYl_f132(h00,Yl,Hl,V0Hl,delV0Yl0Hl_t);
            }
        }

        return;
    }
}

//ф.166
OutData::V ParamsVsr_Yl__5p::GetV0Yl_166f(double h00, double Yl, double Hl, OutData::VLayer V0Hl, OutData::V VYl){
   OutData::V V0Yl;
   V0Yl.VZ=(V0Hl.Vsr.VZ*(Hl-h00)+0.5*(V0Hl.Vd.VZ+VYl.VZ)*(Yl-Hl))/(Yl-h00);
   V0Yl.VM=(V0Hl.Vsr.VM*(Hl-h00)+0.5*(V0Hl.Vd.VM+VYl.VM)*(Yl-Hl))/(Yl-h00);
   return V0Yl;
}

//ф.167
OutData::V ParamsVsr_Yl__5p::GetV0Yl_167f(OutData::V V0Hlsr, OutData::V delV0Yl0Hl){
    OutData::V V0Yl;
    V0Yl.VZ=V0Hlsr.VZ+delV0Yl0Hl.VZ;
    V0Yl.VM=V0Hlsr.VM+delV0Yl0Hl.VM;
    return V0Yl;
}

//ф.168
OutData::V ParamsVsr_Yl__5p::GetV0Yl_168f(double h00, double Yl, double HH, OutData::VLayer V0HH, OutData::V VYl){
    OutData::V V0Yl;
    V0Yl.VZ=(V0HH.Vsr.VZ*(HH-h00)+0.5*(V0HH.Vd.VZ+VYl.VZ)*(Yl-HH))/(Yl-h00);
    V0Yl.VM=(V0HH.Vsr.VM*(HH-h00)+0.5*(V0HH.Vd.VM+VYl.VM)*(Yl-HH))/(Yl-h00);
    return V0Yl;
}


