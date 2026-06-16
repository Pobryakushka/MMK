#include "selectmod__4_5p.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"

#include <iostream>

#include "FunctionsCalc/mathfunc.h"

SelectMod__4_5p::SelectMod__4_5p()
{

}

void SelectMod__4_5p::Get3Mod(int NN, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    int numMod=ParamsMatrix__4_4p::numMod;
    double SUMZ[ParamsMatrix__4_4p::numMod],SUMM[ParamsMatrix__4_4p::numMod];
    for(int j=0;j<numMod;j++){//160
        SUMZ[j]=0;
        SUMM[j]=0;
        for(int i=0;i<NN;i++){
            SUMZ[j]+=MZ[j][i].nm;
            SUMM[j]+=MM[j][i].nm;
        }
    }
    //!!!Для проверки
    //--------------------
    std::cout<<"SUMZ"<<std::endl;
    for(int j=0;j<numMod;j++)
        std::cout<<SUMZ[j]<<"\t";
    std::cout<<std::endl;
    std::cout<<"SUMM"<<std::endl;
    for(int j=0;j<numMod;j++)
        std::cout<<SUMM[j]<<"\t";
    std::cout<<std::endl;
    //---------------------

    MathFunc::Rank RankSumZ[numMod], RankSumM[numMod];
   //Ранжирование моделей по возрастанию номеров мест
    MathFunc::RankingIncrMas1(SUMZ, numMod, RankSumZ);
    MathFunc::RankingIncrMas1(SUMM, numMod, RankSumM);

   // std::cout<<std::endl;
  for (int m=0;m<3;m++){
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VZ=MZ[RankSumZ[m].ind][0].delV;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VM=MM[RankSumM[m].ind][0].delV;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsZ=MZ[RankSumZ[m].ind][0].Eps;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsM=MM[RankSumM[m].ind][0].Eps;

    ParamsdelV__4_3p::ComplMode[m].delV0Yl0Hl.VZ=MZ[RankSumZ[m].ind][0].delV;
    ParamsdelV__4_3p::ComplMode[m].delV0Yl0Hl.VM=MM[RankSumM[m].ind][0].delV;
    ParamsdelV__4_3p::ComplMode[m].Eps.EpsZ=MZ[RankSumZ[m].ind][0].Eps;
    ParamsdelV__4_3p::ComplMode[m].Eps.EpsM=MM[RankSumM[m].ind][0].Eps;
//    //Для проверки
//    std::cout<<"delVZ "<<ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VZ<<
//               " delVM "<<ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VM<<
//               " EpsZ "<<ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsZ<<
//               " EpsM "<<ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsM<<std::endl;
//    //---------------
  }
//  std::cout<<std::endl;
 // free(RankSumZ);

}



