#include "paramsvd_yl__4_2p.h"
#include "FunctionsCalc/functionscalc.h"

ParamsVd_Yl__4_2p::ParamsVd_Yl__4_2p()
{
}

OutData::V ParamsVd_Yl__4_2p::GetVdYl(PlowAlgoritm::ProfilePoint* ProfRPV, int m, int mpl1, double Yl){
   OutData::V VYl;
   FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,m,mpl1,Yl,VYl);
   return VYl;
}
