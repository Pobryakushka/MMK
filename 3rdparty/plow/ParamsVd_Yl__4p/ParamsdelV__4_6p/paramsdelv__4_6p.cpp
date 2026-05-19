#include "paramsdelv__4_6p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"
#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "FunctionsCalc/functionscalc.h"

#include <iostream>

ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p4;

ParamsdelV__4_6p::ParamsdelV__4_6p()
{

}

//after_4_3=true, если вход в блок 4.6 после 4.3
//after_4_3=false, если вход в блок 4.6 после 4.5
OutData::V ParamsdelV__4_6p::GetdelV(int kmod,bool after_4_3,int NN, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    OutData::V delV0Yl0Hl;
    if(after_4_3)
       delV0Yl0Hl=delVafter_4_3(kmod);
    else{
        delV0Yl0Hl=delVafter_4_5(kmod,NN,MZ,MM);
    }
    if (kmod==1){
        if((ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&&
                (ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VM!=Constants::UNDEF_V))
           ParamsVdVsr_Hl__4_1p::IPRL=1;
        else ParamsVdVsr_Hl__4_1p::IPRL=0;
    }
    if(kmod>1) ParamsVdVsr_Hl__4_1p::IPRL=1;
    return delV0Yl0Hl;
}

//Заходила!
OutData::V ParamsdelV__4_6p::delVafter_4_3(int kmod){
    OutData::V delV0Yl0Hl;
    if (kmod==1){
        if((ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&&
                (ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VM!=Constants::UNDEF_V))
        delV0Yl0Hl=ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl;//128
        else delV0Yl0Hl=ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl;
        return delV0Yl0Hl;//переход к блоку 4.7
    }
    bool delV1=((ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&&
            (ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VM!=Constants::UNDEF_V));
    bool delV2=((ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&&
            (ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VM!=Constants::UNDEF_V));
    bool delV3=((ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&&
            (ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VM!=Constants::UNDEF_V));
    if (kmod==2){
        if (delV1 && delV2)
              delV0Yl0Hl=GetdelV1_f129();
        if (delV1 && delV3)
              delV0Yl0Hl=GetdelV2_f130();
        if (delV2 && delV3)
            delV0Yl0Hl=GetdelV12_f130a();

    }
    if (kmod==3)
       delV0Yl0Hl=GetdelV3_f131();
    return delV0Yl0Hl;
}

OutData::V ParamsdelV__4_6p::delVafter_4_5(int kmod, int NN,ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
   OutData::V delV0Yl0Hl;
  // int numMod=ParamsMatrix__4_4p::numMod;
   if (NN>1)
      delV0Yl0Hl=GetdelV3_f131();//165
   if (NN==1){
      if (kmod<=3)
          delV0Yl0Hl=delVkmod3(kmod,MZ,MM);
      else switch (kmod) {
      case 4:
          delV0Yl0Hl=delVkmod4(kmod, MZ, MM);
          break;
      case 5:
          delV0Yl0Hl=delVkmod5(kmod, MZ, MM);
          break;
      case 6:
          delV0Yl0Hl=delVkmod6(kmod, MZ, MM);
          break;
      case 7:
          delV0Yl0Hl=delVkmod7(kmod, MZ, MM);
          break;
      default:
          break;
      }
   }
   return delV0Yl0Hl;
}

OutData::V ParamsdelV__4_6p::delVkmod3(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
   OutData::V delV;
   int k=0;
   for(int j=0;j<ParamsMatrix__4_4p::numMod;j++){
   {
       ParamsVdVsr_0_200m__3_1p::ComplMode[k].V0200.VZ=MZ[j][0].delV;
       ParamsVdVsr_0_200m__3_1p::ComplMode[k].V0200.VM=MM[j][0].delV;
       ParamsVdVsr_0_200m__3_1p::ComplMode[k].Eps.EpsZ=MZ[j][0].Eps;
       ParamsVdVsr_0_200m__3_1p::ComplMode[k].Eps.EpsM=MM[j][0].Eps;
       k++;
    }
   }
   delV=delVafter_4_3(kmod);
   return delV;
}

//161
OutData::V ParamsdelV__4_6p::delVkmod4(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    OutData::V delV;
    delV.VZ=0; delV.VM=0;
    ParamsdelV__4_3p::Mode masMode[kmod];
    masMode[0].P.PZ=0.4;masMode[0].P.PM=0.4;
    masMode[1].P.PZ=0.3;masMode[1].P.PM=0.3;
    masMode[2].P.PZ=0.2;masMode[2].P.PM=0.2;
    masMode[3].P.PZ=0.1;masMode[3].P.PM=0.1;
    int k=0;
    for (int j=0;j<ParamsMatrix__4_4p::numMod;j++){
        if(k==kmod) break;
        if(MZ[j][0].Eps!=ParamsMatrix__4_4p::epsNoMod){
            masMode[MZ[j][0].nm-1].delV0Yl0Hl.VZ=MZ[j][0].delV;
            masMode[MM[j][0].nm-1].delV0Yl0Hl.VM=MM[j][0].delV;
            masMode[MZ[j][0].nm-1].Eps.EpsZ=MZ[j][0].Eps;
            masMode[MM[j][0].nm-1].Eps.EpsM=MM[j][0].Eps;
            k++;
        }
    }
    for(int j=0;j<kmod;j++){
      delV.VZ+=masMode[j].P.PZ*masMode[j].delV0Yl0Hl.VZ;
      delV.VM+=masMode[j].P.PM*masMode[j].delV0Yl0Hl.VM;
    }
    return delV;
}

//162
OutData::V ParamsdelV__4_6p::delVkmod5(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    OutData::V delV;
    delV.VZ=0; delV.VM=0;
    ParamsdelV__4_3p::Mode masMode[kmod];
    masMode[0].P.PZ=0.33;masMode[0].P.PM=0.33;
    masMode[1].P.PZ=0.27;masMode[1].P.PM=0.27;
    masMode[2].P.PZ=0.20;masMode[2].P.PM=0.20;
    masMode[3].P.PZ=0.13;masMode[3].P.PM=0.13;
    masMode[4].P.PZ=0.07;masMode[4].P.PM=0.07;
    int k=0;
    for (int j=0;j<ParamsMatrix__4_4p::numMod;j++){
        if(k==kmod) break;
        if(MZ[j][0].Eps!=ParamsMatrix__4_4p::epsNoMod){
            masMode[MZ[j][0].nm-1].delV0Yl0Hl.VZ=MZ[j][0].delV;
            masMode[MM[j][0].nm-1].delV0Yl0Hl.VM=MM[j][0].delV;
            masMode[MZ[j][0].nm-1].Eps.EpsZ=MZ[j][0].Eps;
            masMode[MM[j][0].nm-1].Eps.EpsM=MM[j][0].Eps;
            k++;
        }
    }
    for(int j=0;j<kmod;j++){
      delV.VZ+=masMode[j].P.PZ*masMode[j].delV0Yl0Hl.VZ;
      delV.VM+=masMode[j].P.PM*masMode[j].delV0Yl0Hl.VM;
    }
    return delV;
}

//163
OutData::V ParamsdelV__4_6p::delVkmod6(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    OutData::V delV;
    delV.VZ=0; delV.VM=0;
    ParamsdelV__4_3p::Mode masMode[kmod];
    masMode[0].P.PZ=0.29;masMode[0].P.PM=0.29;
    masMode[1].P.PZ=0.24;masMode[1].P.PM=0.24;
    masMode[2].P.PZ=0.19;masMode[2].P.PM=0.19;
    masMode[3].P.PZ=0.14;masMode[3].P.PM=0.14;
    masMode[4].P.PZ=0.09;masMode[4].P.PM=0.09;
    masMode[5].P.PZ=0.05;masMode[5].P.PM=0.05;
    int k=0;
    for (int j=0;j<ParamsMatrix__4_4p::numMod;j++){
        if(k==kmod) break;
        if(MZ[j][0].Eps!=ParamsMatrix__4_4p::epsNoMod){
            masMode[MZ[j][0].nm-1].delV0Yl0Hl.VZ=MZ[j][0].delV;
            masMode[MM[j][0].nm-1].delV0Yl0Hl.VM=MM[j][0].delV;
            masMode[MZ[j][0].nm-1].Eps.EpsZ=MZ[j][0].Eps;
            masMode[MM[j][0].nm-1].Eps.EpsM=MM[j][0].Eps;
            k++;
        }
    }
    for(int j=0;j<kmod;j++){
      delV.VZ+=masMode[j].P.PZ*masMode[j].delV0Yl0Hl.VZ;
      delV.VM+=masMode[j].P.PM*masMode[j].delV0Yl0Hl.VM;
    }
    return delV;
}

//164
OutData::V ParamsdelV__4_6p::delVkmod7(int kmod, ParamsMatrix__4_4p::M **MZ, ParamsMatrix__4_4p::M **MM){
    OutData::V delV;
    delV.VZ=0; delV.VM=0;
    ParamsdelV__4_3p::Mode masMode[kmod];
    masMode[0].P.PZ=0.25;masMode[0].P.PM=0.25;
    masMode[1].P.PZ=0.21;masMode[1].P.PM=0.21;
    masMode[2].P.PZ=0.18;masMode[2].P.PM=0.18;
    masMode[3].P.PZ=0.14;masMode[3].P.PM=0.14;
    masMode[4].P.PZ=0.11;masMode[4].P.PM=0.11;
    masMode[5].P.PZ=0.07;masMode[5].P.PM=0.07;
    masMode[6].P.PZ=0.04;masMode[6].P.PM=0.04;
    int k=0;
    for (int j=0;j<ParamsMatrix__4_4p::numMod;j++){
        if(k==kmod) break;
        if(MZ[j][0].Eps!=ParamsMatrix__4_4p::epsNoMod){
            masMode[MZ[j][0].nm-1].delV0Yl0Hl.VZ=MZ[j][0].delV;
            masMode[MM[j][0].nm-1].delV0Yl0Hl.VM=MM[j][0].delV;
            masMode[MZ[j][0].nm-1].Eps.EpsZ=MZ[j][0].Eps;
            masMode[MM[j][0].nm-1].Eps.EpsM=MM[j][0].Eps;
            k++;
        }
    }
    for(int j=0;j<kmod;j++){
      delV.VZ+=masMode[j].P.PZ*masMode[j].delV0Yl0Hl.VZ;
      delV.VM+=masMode[j].P.PM*masMode[j].delV0Yl0Hl.VM;
    }
    return delV;
}

OutData::V ParamsdelV__4_6p::GetdelV1_f129(){
    OutData::V DelV1;
    FunctionsCalc::delV delV0Yl0Hl1,delV0Yl0Hl2;
    /*delV0Yl0Hl1.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ;
    delV0Yl0Hl1.M=ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM;
    delV0Yl0Hl2.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[1].Eps.EpsZ;
    delV0Yl0Hl2.M=ParamsVdVsr_0_200m__3_1p::ComplMode[1].Eps.EpsM;*/
    delV0Yl0Hl1.Z=ParamsdelV__4_3p::ComplMode[0].Eps.EpsZ;
    delV0Yl0Hl1.M=ParamsdelV__4_3p::ComplMode[0].Eps.EpsM;
    delV0Yl0Hl2.Z=ParamsdelV__4_3p::ComplMode[1].Eps.EpsZ;
    delV0Yl0Hl2.M=ParamsdelV__4_3p::ComplMode[1].Eps.EpsM;
    ParamsVdVsr_0_200m__3_1p::IPR _IPR=paramsVdVsr_0_200m__3_1p4.GetIPR(delV0Yl0Hl1,delV0Yl0Hl2);
    Calc_error::P P72[2];
    paramsVdVsr_0_200m__3_1p4.GetP72(_IPR,P72);

   /* DelV1.VZ=P72[0].PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VZ+P72[1].PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VZ;//129
    DelV1.VM=P72[0].PM*ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VM+P72[1].PM*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VM;*/

//    DelV1.VZ=P72[0].PZ*ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VZ+P72[1].PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VZ;//129
//    DelV1.VM=P72[0].PM*ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VM+P72[1].PM*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VM;

    DelV1.VZ=P72[0].PZ*ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VZ+P72[1].PZ*ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VZ;//129
    DelV1.VM=P72[0].PM*ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VM+P72[1].PM*ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VM;
    return DelV1;
}

OutData::V ParamsdelV__4_6p::GetdelV2_f130(){
    OutData::V DelV2;
    FunctionsCalc::delV delV0Yl0Hl1,delV0Yl0Hl3;
    FunctionsCalc::delV eps1, eps3;
    /*delV0Yl0Hl1.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VZ;
    delV0Yl0Hl1.M=ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VM;
    delV0Yl0Hl3.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VZ;
    delV0Yl0Hl3.M=ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VM;
    eps1.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ;
    eps1.M=ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM;
    eps3.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsZ;
    eps3.M=ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsM;*/
    delV0Yl0Hl1.Z=ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VZ;
        delV0Yl0Hl1.M=ParamsdelV__4_3p::ComplMode[0].delV0Yl0Hl.VM;
        delV0Yl0Hl3.Z=ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VZ;
        delV0Yl0Hl3.M=ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VM;
        eps1.Z=ParamsdelV__4_3p::ComplMode[0].Eps.EpsZ;
        eps1.M=ParamsdelV__4_3p::ComplMode[0].Eps.EpsM;
        eps3.Z=ParamsdelV__4_3p::ComplMode[2].Eps.EpsZ;
        eps3.M=ParamsdelV__4_3p::ComplMode[2].Eps.EpsM;
    ParamsVdVsr_0_200m__3_1p::IPR _IPR=paramsVdVsr_0_200m__3_1p4.GetIPR(eps1,eps3);
    Calc_error::P P72[2];
    paramsVdVsr_0_200m__3_1p4.GetP72(_IPR, P72);

    DelV2.VZ=P72[0].PZ*delV0Yl0Hl1.Z+P72[1].PZ*delV0Yl0Hl3.Z;//130
    DelV2.VM=P72[0].PM*delV0Yl0Hl1.M+P72[1].PM*delV0Yl0Hl3.M;
    return DelV2;
}

OutData::V ParamsdelV__4_6p::GetdelV12_f130a(){
    OutData::V DelV1;
    FunctionsCalc::delV delV0Yl0Hl1,delV0Yl0Hl2;
  /*  delV0Yl0Hl1.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[1].Eps.EpsZ;
    delV0Yl0Hl1.M=ParamsVdVsr_0_200m__3_1p::ComplMode[1].Eps.EpsM;
    delV0Yl0Hl2.Z=ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsZ;
    delV0Yl0Hl2.M=ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsM;*/
        delV0Yl0Hl1.Z=ParamsdelV__4_3p::ComplMode[1].Eps.EpsZ;
        delV0Yl0Hl1.M=ParamsdelV__4_3p::ComplMode[1].Eps.EpsM;
        delV0Yl0Hl2.Z=ParamsdelV__4_3p::ComplMode[2].Eps.EpsZ;
        delV0Yl0Hl2.M=ParamsdelV__4_3p::ComplMode[2].Eps.EpsM;
    ParamsVdVsr_0_200m__3_1p::IPR _IPR=paramsVdVsr_0_200m__3_1p4.GetIPR(delV0Yl0Hl1,delV0Yl0Hl2);
    Calc_error::P P72[2];
    paramsVdVsr_0_200m__3_1p4.GetP72(_IPR,P72);
   /* DelV1.VZ=P72[0].PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VZ+P72[1].PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VZ;//130a(возможно, ее нет по тексту алгоритма)
    DelV1.VM=P72[0].PM*ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VM+P72[1].PM*ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200.VM;*/
    DelV1.VZ=P72[0].PZ*ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VZ+P72[1].PZ*ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VZ;//130a(возможно, ее нет по тексту алгоритма)
    DelV1.VM=P72[0].PM*ParamsdelV__4_3p::ComplMode[1].delV0Yl0Hl.VM+P72[1].PM*ParamsdelV__4_3p::ComplMode[2].delV0Yl0Hl.VM;
    return DelV1;
}


//56
bool ParamsdelV__4_6p::GetPi1i2i3(int i1, int i2, int i3){
    try{
//        ParamsVdVsr_0_200m__3_1p::EpsInd *EpsIndZM;
//        EpsIndZM=paramsVdVsr_0_200m__3_1p4.GetEpsInd(i1, i2, i3);
        ParamsVdVsr_0_200m__3_1p::EpsInd EpsIndZM[3];
        paramsVdVsr_0_200m__3_1p4.GetEpsInd(i1, i2, i3,EpsIndZM);
//                //Для проверки
//                for(int i=0;i<3;i++){
//                    std::cout<<"delVZ "<<ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VZ<<
//                               " delVM "<<ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VM<<
//                               " EpsZ "<<ParamsVdVsr_0_200m__3_1p::ComplMode[i].Eps.EpsZ<<
//                               " EpsM "<<ParamsVdVsr_0_200m__3_1p::ComplMode[i].Eps.EpsM<<
//                                std::endl;
//                }
//        //------------------
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].Z.ind].P.PZ=0.5;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].Z.ind].P.PZ=0.33;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].Z.ind].P.PZ=0.17;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].M.ind].P.PM=0.5;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].M.ind].P.PM=0.33;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].M.ind].P.PM=0.17;
        return true;
    }
    catch(...){
        return false;
    }
}

OutData::V ParamsdelV__4_6p::GetdelV3_f131(){
    OutData::V DelV3;
    GetPi1i2i3(1,2,3);
    DelV3.VZ=0;DelV3.VM=0;
    for (int i=0;i<3;i++){
    /* DelV3.VZ+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VZ;//131
      DelV3.VM+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PM*ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VM;*/
        DelV3.VZ+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PZ*ParamsdelV__4_3p::ComplMode[i].delV0Yl0Hl.VZ;//131  !!!
        DelV3.VM+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PM*ParamsdelV__4_3p::ComplMode[i].delV0Yl0Hl.VM;
    }
    //Для проверки
    std::cout<<std::endl;
    std::cout<<" DelVZ "<<DelV3.VZ<<std::endl;
    std::cout<<" DelVM "<<DelV3.VM<<std::endl;
    std::cout<<std::endl;
    //------------------
    return DelV3;
}



