#include "paramsvdvsr_0_200m__3_1p.h"
#include "FunctionsCalc/calc_bottomlayer.h"
#include "InData/indataclimat.h"
#include "InData/Constants.h"
//#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "BottomEval__2p/bottomeval__2p.h"
#include "math.h"
#include <iostream>


ParamsVdVsr_0_200m__3_1p::Mode ParamsVdVsr_0_200m__3_1p::ComplMode[7];
BottomEval__2p bottomEval__2p1;

ParamsVdVsr_0_200m__3_1p::ParamsVdVsr_0_200m__3_1p()
{
}

bool ParamsVdVsr_0_200m__3_1p::Do_3_1(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, OutData::VLayer &V0200,
  OutData::VLayer &V0100, OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V150,
                                      OutData::V **VCast){
    try{
        /*this->ProfRPV = ProfRPV;
        this->cl=cl;*/
        //Рассчитываем средний ветер в слое 0-200м
        Get_Vsr_0_200(ProfRPV,cl,V0200,  V0100, V050,VCast);
        FunctionsCalc::GetVTeta_LineInterpol3(InData::V0ZM,V050.Vd,InData::h0,
                         Constants::level50,Constants::level25,V025.Vd);
        FunctionsCalc::GetVTeta_LineInterpol4(V050.Vd,V0100.Vd,V075.Vd);
        FunctionsCalc::GetVTeta_LineInterpol4(V0100.Vd,V0200.Vd,V150.Vd);
        return true;
    }
    catch(...){
        return false;
    }
}

 //Рассчитывает средний ветер в слое 0-200м
bool ParamsVdVsr_0_200m__3_1p::Get_Vsr_0_200(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050,
                                             OutData::V **VCast){
    try{
        Get_Vsr_0_200_1v(ProfRPV,cl);
        Get_Vsr_0_200_2v(ProfRPV,cl);
        Get_Vsr_0_200_3v(ProfRPV, cl);
        Get_Vsr_0_200_4v(VCast);

        int countmode=0;
        char comb[4]="";
        char str[4]="";
        std::cout<<"ComplMode"<<std::endl;
        for (int i=0; i<4;i++){
            if (ParamsVdVsr_0_200m__3_1p::ComplMode[i].Eps.EpsZ>=0){
                countmode++;
//                itoa((i+1),str,10);
                sprintf(str, "%d", (i+1));
                strcat(comb,str);
                std::cout<<"V\t"<<ComplMode[i].V0200.VZ<<"\t"<<ComplMode[i].V0200.VM<<
                "\tEps\t"<<ComplMode[i].Eps.EpsZ<<"\t"<<ComplMode[i].Eps.EpsM<<std::endl;
            }
        }
        GetP(comb);
        OutData::V V0200sr;
        GetVZ0200(V0200sr);

        //ф.61
        OutData::V V0100_1;
        V0100_1.VZ=V0200sr.VZ*0.82;
        V0100_1.VM=V0200sr.VM*0.83;

        //ф.62
        OutData::V V200_1;
        V200_1.VZ=3*V0200sr.VZ-2*V0100_1.VZ;
        V200_1.VM=3*V0200sr.VM-2*V0100_1.VM;

        //ф.63
        double V50=FunctionsCalc::IndiffStrat(Constants::level50,InData::V0_tau0,InData::h0);
        double TETA50=InData::TETA0_tau0;
        double V100=FunctionsCalc::IndiffStrat(Constants::level100,InData::V0_tau0,InData::h0);
        double TETA100=InData::TETA0_tau0+6;

        //ф.64
        OutData::V V50_1, V100_1;
        FunctionsCalc::Get_VZ_VM(V50,TETA50,V50_1);
        FunctionsCalc::Get_VZ_VM(V100,TETA100,V100_1);

        //ф.65
        OutData::V V0;
        FunctionsCalc::Get_VZ_VM(InData::V0_tau0,InData::TETA0_tau0,V0);
        OutData::V V0100_2;
        V0100_2.VZ=(V0.VZ+V100_1.VZ)/2;
        V0100_2.VM=(V0.VM+V100_1.VM)/2;

        //ф.66
        OutData::V V200_2;
        V200_2.VZ=3*V0200sr.VZ-2*V0100_2.VZ;
        V200_2.VM=3*V0200sr.VM-2*V0100_2.VM;

        //ф.67
         OutData::V V100_t;
         V100_t.VZ=(V0.VZ+V200_1.VZ)/2;
         V100_t.VM=(V0.VM+V200_1.VM)/2;

         //ф.68
         OutData::V V100_t_sh;
         V100_t_sh.VZ=(V0.VZ+V200_2.VZ)/2;
         V100_t_sh.VM=(V0.VM+V200_2.VM)/2;

         //ф.69, 70
         FunctionsCalc::delV delV_t= FunctionsCalc::GetdelV(V100_t,V100_1);
         FunctionsCalc::delV delV_t_sh= FunctionsCalc::GetdelV(V100_t_sh,V100_1);

        // Calc_error::P *P72;
         IPR _IPR=GetIPR(delV_t,delV_t_sh);

         //P72=GetP72(_IPR);
         Calc_error::P P72[2];
         GetP72(_IPR,P72);

         //ф.71
         V0200.Vd.VZ=P72[0].PZ*V200_1.VZ+P72[1].PZ*V200_2.VZ;
         V0200.Vd.VM=P72[0].PM*V200_1.VM+P72[1].PM*V200_2.VM;

         //ф.73
         OutData::V V100_2;
         V100_2.VZ=2*V0100_1.VZ-V0.VZ;
         V100_2.VM=2*V0100_1.VM-V0.VM;

         Calc_error::P P75[2];
         GetP75(_IPR,P75);

         //ф.74
         V0100.Vd.VZ=P75[0].PZ*V100_1.VZ+P75[1].PZ*V100_2.VZ;
         V0100.Vd.VM=P75[0].PM*V100_1.VM+P75[1].PM*V100_2.VM;

         //ф.76
         OutData::V V50_2;
         V50_2.VZ=(V0.VZ+V100_2.VZ)/2;
         V50_2.VM=(V0.VM+V100_2.VM)/2;

         //ф.77,78,79
         V050.Vd.VZ=P75[0].PZ*V50_1.VZ+P75[1].PZ*V50_2.VZ;
         V050.Vd.VM=P75[0].PM*V50_1.VM+P75[1].PM*V50_2.VM;
         V0100.Vsr.VZ=P72[0].PZ*V0100_1.VZ+P72[1].PZ*V0100_2.VZ;
         V0100.Vsr.VM=P72[0].PM*V0100_1.VM+P72[1].PM*V0100_2.VM;
         V050.Vsr.VZ=(V0.VZ+V050.Vd.VZ)/2;
         V050.Vsr.VM=(V0.VM+V050.Vd.VM)/2;
         V0200.Vsr.VZ=V0200sr.VZ;
         V0200.Vsr.VM=V0200sr.VM;

        return true;
    }
    catch(...){
        return false;
    }
}

ParamsVdVsr_0_200m__3_1p::IPR ParamsVdVsr_0_200m__3_1p::GetIPR(FunctionsCalc::delV delVt, FunctionsCalc::delV delVtsh){
    IPR _IPR;
    if (delVtsh.Z==delVt.Z) _IPR.Z=0;
    if (delVtsh.Z<delVt.Z) _IPR.Z=1;
    if (delVtsh.Z>delVt.Z) _IPR.Z=2;
    if (delVtsh.M==delVt.M) _IPR.M=0;
    if (delVtsh.M<delVt.M) _IPR.M=1;
    if (delVtsh.M>delVt.M) _IPR.M=2;
    return _IPR;
}

//Получает весовые коэффициенты по ф.72
void ParamsVdVsr_0_200m__3_1p::GetP72(ParamsVdVsr_0_200m__3_1p::IPR IPR, Calc_error::P *P ){
  switch (IPR.Z) {
  case 0:
      P[0].PZ=0.5;
      P[1].PZ=0.5;
      break;
  case 1:
      P[0].PZ=0.33;
      P[1].PZ=0.67;
      break;
  case 2:
      P[0].PZ=0.67;
      P[1].PZ=0.33;
      break;
  default:
      break;
  }
  switch (IPR.M) {
  case 0:
      P[0].PM=0.5;
      P[1].PM=0.5;
      break;
  case 1:
      P[0].PM=0.33;
      P[1].PM=0.67;
      break;
  case 2:
      P[0].PM=0.67;
      P[1].PM=0.33;
      break;
  default:
      break;
  }
 // return P;
}

//Получает весовые коэффициенты по ф.75
void ParamsVdVsr_0_200m__3_1p::GetP75(ParamsVdVsr_0_200m__3_1p::IPR IPR, Calc_error::P *P){
  switch (IPR.Z) {
  case 0:
      P[0].PZ=0.5;
      P[1].PZ=0.5;
      break;
  case 1:
      P[0].PZ=0.67;
      P[1].PZ=0.33;
      break;
  case 2:
      P[0].PZ=0.33;
      P[1].PZ=0.67;
      break;
  default:
      break;
  }
  switch (IPR.M) {
  case 0:
      P[0].PM=0.5;
      P[1].PM=0.5;
      break;
  case 1:
      P[0].PM=0.67;
      P[1].PM=0.33;
      break;
  case 2:
      P[0].PM=0.33;
      P[1].PM=0.67;
      break;
  default:
      break;
  }
  //return P;
}

//Получает комплекированные по 4-м способам значения зональной и
//меридиальной составляющей среднего ветра в слое 0-200м
//ф.60
bool ParamsVdVsr_0_200m__3_1p::GetVZ0200(OutData::V &VZ0200){
    try{
      //  std::cout<<"GetVZ0200"<<std::endl;
        VZ0200.VZ=0; VZ0200.VM=0;
        for(int i=0;i<4;i++){
         VZ0200.VZ+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PZ*ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VZ;
         VZ0200.VM+=ParamsVdVsr_0_200m__3_1p::ComplMode[i].P.PM*ParamsVdVsr_0_200m__3_1p::ComplMode[i].V0200.VM;
//         std::cout<<"V\t"<<ComplMode[i].V0200.VZ<<"\t"<<ComplMode[i].V0200.VM<<
//         "\tP\t"<<ComplMode[i].P.PZ<<"\t"<<ComplMode[i].P.PM<<std::endl;
        }
        return true;
    }
    catch(...){
        return false;
    }
}

//Определяет весовые коэффициенты каждого из способов
bool ParamsVdVsr_0_200m__3_1p::GetP(char *comb){
    try{
        if (strcmp(comb,"13")==0)
           GetP13();
        if (strcmp(comb,"123")==0)
           GetPi1i2i3(1,2,3);
        if (strcmp(comb,"134")==0)
           GetPi1i2i3(1,3,4);
        if (strcmp(comb,"1234")==0)
           GetPi1i2i3i4();


        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_1p::GetP13(){
    try{
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ==ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsZ){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PZ=0.5; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PZ=0.5;
        }
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ<ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsZ){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PZ=0.67; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PZ=0.33;
        }
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ>ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsZ){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PZ=0.33; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PZ=0.67;
        }
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM==ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsM){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PM=0.5; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PM=0.5;
        }
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM<ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsM){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PM=0.67; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PM=0.33;
        }
        if (ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM>ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps.EpsM){
             ParamsVdVsr_0_200m__3_1p::ComplMode[0].P.PM=0.33; ParamsVdVsr_0_200m__3_1p::ComplMode[2].P.PM=0.67;
        }
        return true;
    }
    catch(...){
        return false;
    }
}

//56
bool ParamsVdVsr_0_200m__3_1p::GetPi1i2i3(int i1, int i2, int i3){
    try{
       // EpsInd *EpsIndZM;
        EpsInd EpsIndZM[3];
       // EpsIndZM=GetEpsInd(i1, i2, i3);
        GetEpsInd(i1, i2, i3,EpsIndZM);
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].Z.ind].P.PZ=0.5;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].Z.ind].P.PZ=0.33;
        ParamsVdVsr_0_200m__3_1p:: ComplMode[EpsIndZM[2].Z.ind].P.PZ=0.17;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].M.ind].P.PM=0.5;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].M.ind].P.PM=0.33;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].M.ind].P.PM=0.17;
        return true;
    }
    catch(...){
        return false;
    }
}

//ф.59
bool ParamsVdVsr_0_200m__3_1p::GetPi1i2i3i4(){
    try{
        EpsInd EpsIndZM[4];
        GetEpsInd(EpsIndZM);
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].Z.ind].P.PZ=0.4;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].Z.ind].P.PZ=0.3;
        ParamsVdVsr_0_200m__3_1p:: ComplMode[EpsIndZM[2].Z.ind].P.PZ=0.2;
        ParamsVdVsr_0_200m__3_1p:: ComplMode[EpsIndZM[3].Z.ind].P.PZ=0.1;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].M.ind].P.PM=0.4;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].M.ind].P.PM=0.3;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].M.ind].P.PM=0.2;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[3].M.ind].P.PM=0.1;
        return true;
    }
    catch(...){
        return false;
    }
}

//Ранжирует ошибки по возрастанию
void ParamsVdVsr_0_200m__3_1p::GetEpsInd(int i1, int i2, int i3,EpsInd *EpsIndZM){
   // EpsInd *EpsIndZM=new EpsInd[3];
    try{
       EpsIndZM[0].Z.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i1-1].Eps.EpsZ;
       EpsIndZM[0].Z.ind=i1-1;
       EpsIndZM[1].Z.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i2-1].Eps.EpsZ;
       EpsIndZM[1].Z.ind=i2-1;
       EpsIndZM[2].Z.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i3-1].Eps.EpsZ;
       EpsIndZM[2].Z.ind=i3-1;
       EpsIndZM[0].M.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i1-1].Eps.EpsM;
       EpsIndZM[0].M.ind=i1-1;
       EpsIndZM[1].M.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i2-1].Eps.EpsM;
       EpsIndZM[1].M.ind=i2-1;
       EpsIndZM[2].M.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i3-1].Eps.EpsM;
       EpsIndZM[2].M.ind=i3-1;

       double eps;
       int ind;
       for (int j=0;j<3;j++){
           for(int i=0;i<2;i++){
              if (EpsIndZM[i+1].Z.eps<EpsIndZM[i].Z.eps){
                  eps=EpsIndZM[i].Z.eps;
                  ind=EpsIndZM[i].Z.ind;
                  EpsIndZM[i].Z.eps=EpsIndZM[i+1].Z.eps;
                  EpsIndZM[i].Z.ind=EpsIndZM[i+1].Z.ind;
                  EpsIndZM[i+1].Z.eps=eps;
                  EpsIndZM[i+1].Z.ind=ind;
              }
              if (EpsIndZM[i+1].M.eps<EpsIndZM[i].M.eps){
                  eps=EpsIndZM[i].M.eps;
                  ind=EpsIndZM[i].M.ind;
                  EpsIndZM[i].M.eps=EpsIndZM[i+1].M.eps;
                  EpsIndZM[i].M.ind=EpsIndZM[i+1].M.ind;
                  EpsIndZM[i+1].M.eps=eps;
                  EpsIndZM[i+1].M.ind=ind;
              }
             }
           }
      // return EpsIndZM;
    }
    catch(...){
        // return EpsIndZM;;
    }
}

//Ранжирует ошибки по возрастанию
//если возможно реализовать вс 4 способа расчета
void ParamsVdVsr_0_200m__3_1p::GetEpsInd(EpsInd *EpsIndZM){
   // EpsInd *EpsIndZM=new EpsInd[3];
    try{
       for(int i=0;i<4;i++){
           EpsIndZM[i].Z.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i].Eps.EpsZ;
           EpsIndZM[i].Z.ind=i;
           EpsIndZM[i].M.eps=ParamsVdVsr_0_200m__3_1p::ComplMode[i].Eps.EpsM;
           EpsIndZM[i].M.ind=i;
       }
       double eps;
       int ind;
       for (int j=0;j<4;j++){
           for(int i=0;i<3;i++){
              if (EpsIndZM[i+1].Z.eps<EpsIndZM[i].Z.eps){
                  eps=EpsIndZM[i].Z.eps;
                  ind=EpsIndZM[i].Z.ind;
                  EpsIndZM[i].Z.eps=EpsIndZM[i+1].Z.eps;
                  EpsIndZM[i].Z.ind=EpsIndZM[i+1].Z.ind;
                  EpsIndZM[i+1].Z.eps=eps;
                  EpsIndZM[i+1].Z.ind=ind;
              }
              if (EpsIndZM[i+1].M.eps<EpsIndZM[i].M.eps){
                  eps=EpsIndZM[i].M.eps;
                  ind=EpsIndZM[i].M.ind;
                  EpsIndZM[i].M.eps=EpsIndZM[i+1].M.eps;
                  EpsIndZM[i].M.ind=EpsIndZM[i+1].M.ind;
                  EpsIndZM[i+1].M.eps=eps;
                  EpsIndZM[i+1].M.ind=ind;
              }
             }
           }
    }
    catch(...){
    }
}

bool ParamsVdVsr_0_200m__3_1p::GetPi1i2i3(){
    try{
        EpsInd EpsIndZM[4];
        GetEpsInd(EpsIndZM);
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].Z.ind].P.PZ=0.4;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].Z.ind].P.PZ=0.3;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].Z.ind].P.PZ=0.2;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[3].Z.ind].P.PZ=0.1;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[0].M.ind].P.PM=0.4;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[1].M.ind].P.PM=0.3;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[2].M.ind].P.PM=0.2;
        ParamsVdVsr_0_200m__3_1p::ComplMode[EpsIndZM[3].M.ind].P.PM=0.1;
        return true;
    }
    catch(...){
        return false;
    }
}

//Рассчитывает средний ветер в слое 0-200м по 1-ому способу
bool ParamsVdVsr_0_200m__3_1p::Get_Vsr_0_200_1v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl){
   try{
       double V_0_200=0;
       double TETA_0_200=0;
       Calc_bottomLayer::Get_V0_0_200(V_0_200, TETA_0_200);
       FunctionsCalc::Get_VZ_VM(V_0_200,TETA_0_200,ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200);
       ClimatData::Sigma Si;
   //!si     Climat::Get_Si(Constants::level200,Climat::Zone,Si);
       Si=cl->sigma(Constants::level200);

       ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsZ=Si.SiVZ;
       ParamsVdVsr_0_200m__3_1p::ComplMode[0].Eps.EpsM=Si.SiVM;
//       std::cout<<"V0200 1s: "<<ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VZ<<"\t"
//               <<ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200.VM<<std::endl;
       return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_1p::Get_Vsr_0_200_2v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl){
   try{
      double h1= ProfRPV[0].height;
//      double h2= ProfRPV[1].height;
//      double h3= ProfRPV[2].height;
        if (InData::k<3)
          return false;
        if(h1>400)
            return false;

        if(!Method0hn1(ProfRPV,cl,1,ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200,
                       ParamsVdVsr_0_200m__3_1p::ComplMode[1].Eps))
            return false;
        return true;
    }
    catch(...){
        return false;
    }
}

//Пересчет значений скорости в зон. и мер. составляющие
void ParamsVdVsr_0_200m__3_1p::GetMasVZM(PlowAlgoritm::ProfilePoint* ProfRPV,OutData::V *VZM){
    double V;
    double TETA;
    for (int k=0;k<InData::k;k++){
            V=ProfRPV[k].speed;
            TETA=ProfRPV[k].azimut;
            if (V==Constants::UNDEF_V){
                VZM[k].VZ=Constants::UNDEF_V;
                VZM[k].VM=Constants::UNDEF_V;
            }
            else
              FunctionsCalc::Get_VZ_VM(V,TETA,VZM[k]);
//         std::cout<<VZM[k].VZ<<" "<<VZM[k].VM<<" "<<std::endl;
//         std::cout<<"\r\n";
     }
}

//Методика для расчета среднего ветра в слое 0-hn1 стр.65
bool ParamsVdVsr_0_200m__3_1p::Method0hn1(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl,int n1, OutData::V &V0200_2,Calc_error::Eps &Eps2){
   OutData::V *VZM=new OutData::V [InData::k];
   GetMasVZM(ProfRPV, VZM);
   int nk=0;
   FunctionsCalc::Gr gr[InData::k];
   std::list<int>::iterator it;
   list<int> MK=FormMK(n1, ProfRPV, VZM,gr, nk);
   if(nk==0) {
       delete [] VZM;
       return false;
   }
   it=MK.begin();
   //Массив параметров среднего ветра в слое 0-200
   OutData::V V0200[nk];
   Calc_error::Eps Eps[nk];
   int i,j;
   for(int c=0;c<nk;c++){
       i=*it;
       ++it;
       j=*it;
       ++it;
       double hi=ProfRPV[i].height;
       double hj=ProfRPV[j].height;
       double hn1=ProfRPV[n1-1].height;
       OutData::V V0hn1;
       OutData::V Vi=VZM[i];
       OutData::V Vj=VZM[j];
       OutData::V Vn1=VZM[n1-1];
       if((hi<hn1) && (hj<hn1))
           V0hn1=GetV0hn1_1v(Vi, Vj, Vn1, hi, hj, hn1);
       if((hi>hn1) && (hj>hn1))
           V0hn1=GetV0hn1_2v(Vi, Vj, Vn1, hi, hj, hn1);
       //if((hi>hn1) && (hj<hn1) || (hi<hn1) && (hj>hn1))
       if(((hi>hn1) && (hj<hn1)) || ((hi<hn1) && (hj>hn1)))
           V0hn1=GetV0hn1_3v(Vi, Vj, Vn1, hi, hj);
       double hmas[3];
       hmas[0]=hi;hmas[1]=hj;hmas[2]=hn1;
       int ind;
       double hh=MathFunc::Poisk_maxd(hmas,3,ind);
       ClimatData::Sigma Si0hh;
       Calc_error::Eps Eps0hn1;
       Climat::Get_Si_h(cl, hh,Si0hh);
      // Eps0hn1=Calc_error::Get_Eps(Si0hh,0,hh,ProfRPV);
        Eps0hn1=Calc_error::Get_Eps(Si0hh,hmas,hh);
       ClimatData::Pr pr0200=GetV0200_f43_43(cl,hn1, V0hn1);
       V0200[c].VZ=pr0200.VZ;
       V0200[c].VM=pr0200.VM;
       Get_Eps_2v(cl,Constants::level200, hn1, Eps0hn1, V0hn1, Eps[c]);

   }

   double V0200Z=0, V0200M=0;
   double epsZ=0, epsM=0;

   for(int c=0;c<nk;c++){
    V0200Z+=V0200[c].VZ;
    V0200M+=V0200[c].VM;
    epsZ+=pow(Eps[c].EpsZ,2);
    epsM+=pow(Eps[c].EpsM,2);
   }
   //ф.45а
   V0200_2.VZ=V0200Z/nk;
   V0200_2.VM=V0200M/nk;
   Eps2.EpsZ=pow(epsZ/nk,0.5);
   Eps2.EpsM=pow(epsM/nk,0.5);

//      cout<<V0200_2.VZ<<"\t"<<V0200_2.VM<<endl;
//      cout<<Eps2.EpsZ<<"\t"<<Eps2.EpsM<<endl;
   delete[] VZM;
   return true;
}

//Формирование множества всевозможных комбинаций по 2 уровня (стр.66)
list<int> ParamsVdVsr_0_200m__3_1p::FormMK(int n1,PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V *VZM, FunctionsCalc::Gr* _gr ,int &nk){
   list<int> MK;
   FunctionsCalc::Gr gr[InData::k];
   for(int i=0;i<InData::k;i++)
    if(i!=(n1-1)){
        gr[i]=FunctionsCalc::GetGr(VZM[i],VZM[n1-1],ProfRPV[i].height,ProfRPV[n1-1].height);
        _gr[i]=gr[i];
    }
//   for(int i=0;i<InData::k;i++)
//       std::cout<<gr[i].Z<<"\t"<<gr[i].M<<std::endl;
   bool b1=false;
   bool b2=false;
   bool b3=false;
   nk=0;//кол-во отобранных комбинаций
   for(int i=0;i<InData::k;i++)
       for(int j=0;j<InData::k;j++){
          b1=(i!=(n1-1));
          b2=(j!=(n1-1));
          b3=(i!=j);
          if(b1 && b2 && b3){
           double hi=ProfRPV[i].height;
           double hj=ProfRPV[j].height;
           double hn1=ProfRPV[n1-1].height;
           bool b4=(hi<=2*hn1)&& (hj<=2*hn1);
           if(b4 || (nk<2)){
             bool b5=(gr[i].Z<=0.0033);
             bool b6=(gr[i].M<=0.0033);
             bool b7=(gr[j].Z<=0.0033);
             bool b8=(gr[j].M<=0.0033);
             if(b5 && b6 && b7 && b8){
                MK.push_back(i);
                MK.push_back(j);
                nk++;
             }
           }

         }
       }
   return MK;
}

//ф.39а
OutData::V ParamsVdVsr_0_200m__3_1p::GetV0hn1_1v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                                 double hi, double hj, double hn1){
    OutData::V V0hn1;
    OutData::V Vm2, Vm1;
    double hm2,hm1;
    if(hi<hj) {
        hm2=hi; hm1=hj;
        Vm2=Vi; Vm1=Vj;
    }
    if(hi>hj){
        hm2=hj; hm1=hi;
        Vm2=Vj; Vm1=Vi;
    }
    V0hn1.VZ=(Vm1.VZ+Vn1.VZ)*(hn1-hm2)+(Vm2.VZ-Vn1.VZ)*hm1;
    V0hn1.VZ=V0hn1.VZ/(2*(hn1-hm2));
    V0hn1.VM=(Vm1.VM+Vn1.VM)*(hn1-hm2)+(Vm2.VM-Vn1.VM)*hm1;
    V0hn1.VM=V0hn1.VM/(2*(hn1-hm2));
    return V0hn1;
}

//ф.39б
OutData::V ParamsVdVsr_0_200m__3_1p::GetV0hn1_2v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                                 double hi, double hj, double hn1){
    OutData::V V0hn1;
    OutData::V Vm2, Vm1;
    double hm2,hm1;
    if(hi<hj) {
        hm2=hj; hm1=hi;
        Vm2=Vj; Vm1=Vi;
    }
    if(hi>hj){
        hm2=hi; hm1=hj;
        Vm2=Vi; Vm1=Vj;
    }
    V0hn1.VZ=(Vm1.VZ+Vn1.VZ)*(hm2-hn1)+(Vn1.VZ-Vm2.VZ)*hm1;
    V0hn1.VZ=V0hn1.VZ/(2*(hm2-hn1));
    V0hn1.VM=(Vm1.VM+Vn1.VM)*(hm2-hn1)+(Vn1.VM-Vm2.VM)*hm1;
    V0hn1.VM=V0hn1.VM/(2*(hm2-hn1));
    return V0hn1;
}

//ф.39в
OutData::V ParamsVdVsr_0_200m__3_1p::GetV0hn1_3v(OutData::V Vi, OutData::V Vj, OutData::V Vn1,
                                                 double hi, double hj){
    OutData::V V0hn1;
    OutData::V Vmpl1, Vm1;
    double hmpl1,hm1;
    if(hi<hj) {
        hmpl1=hj; hm1=hi;
        Vmpl1=Vj; Vm1=Vi;
    }
    if(hi>hj){
        hmpl1=hi; hm1=hj;
        Vmpl1=Vi; Vm1=Vj;
    }
    V0hn1.VZ=(Vm1.VZ+Vn1.VZ)*hmpl1-(Vn1.VZ+Vmpl1.VZ)*hm1;
    V0hn1.VZ=V0hn1.VZ/(2*(hmpl1-hm1));
    V0hn1.VM=(Vm1.VM+Vn1.VM)*hmpl1-(Vn1.VM+Vmpl1.VM)*hm1;
    V0hn1.VM=V0hn1.VM/(2*(hmpl1-hm1));
    return V0hn1;
}

OutData::V ParamsVdVsr_0_200m__3_1p::GetV0200_f43(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, double h1, double h2, double h3, Calc_error::Eps &Eps0h1){
    OutData::V V0200;
    OutData::V Vh1, Vh2, Vh3;
    FunctionsCalc::Get_VZ_VM( ProfRPV[0].speed, ProfRPV[0].azimut, Vh1);
    FunctionsCalc::Get_VZ_VM( ProfRPV[1].speed, ProfRPV[1].azimut, Vh2);
    FunctionsCalc::Get_VZ_VM( ProfRPV[2].speed, ProfRPV[2].azimut, Vh3);

    //формула 39
    OutData::V V0h1;
    V0h1.VZ=((Vh1.VZ+Vh2.VZ)*(h3-h1)+(Vh1.VZ-Vh3.VZ)*h2)/(2*(h3-h1));
    V0h1.VM=((Vh1.VM+Vh2.VM)*(h3-h1)+(Vh1.VM-Vh3.VM)*h2)/(2*(h3-h1));

    ClimatData::Sigma Si0h3;
    Calc_error::Eps Eps0h3;

    Climat::Get_Si_h(cl, h3,Si0h3);
    Eps0h3=Calc_error::Get_Eps(Si0h3,0,h3,ProfRPV);
    Eps0h1=Eps0h3;
    ClimatData::Pr pr0200;
    pr0200=GetV0200_f43_43(cl,h1,V0h1);
    V0200.VZ=pr0200.VZ;
    V0200.VM=pr0200.VM;

    return V0200;
}

ClimatData::Pr ParamsVdVsr_0_200m__3_1p::GetV0200_f43_43(const ClimatData* cl, double h1, OutData::V V0h1){
    ClimatData::Pr V0200;
    ClimatData::Pr Vcl0h1, Vcl0200;

    Climat::Get_Vcl_h(cl,h1,Vcl0h1);
    //!pr Climat::Get_Vcl(Constants::level200,Climat::Zone,Vcl0200);
    Vcl0200=cl->pr(Constants::level200,-1);
    //ф.43
    V0200.VZ=V0h1.VZ-Vcl0h1.VZ+Vcl0200.VZ;
    V0200.VM=V0h1.VM-Vcl0h1.VM+Vcl0200.VM;
    return V0200;
}


//Получает оценки теоретических ошибок 2-ого способа
//ф.45
bool ParamsVdVsr_0_200m__3_1p::Get_Eps_2v(const ClimatData* cl,double level, double h, Calc_error::Eps Eps0h,
                                          OutData::V V0hn1, Calc_error::Eps &Eps){
    try{
        ClimatData::Sigma Si0200;
        //!si Climat::Get_Si(level,Climat::Zone,Si0200);
        Si0200=cl->sigma(level);
        ClimatData::Sigma Si0h;
        Climat::Get_Si_h(cl,h,Si0h);

        ClimatData::Correl Corr;
        //!corr Climat::Get_Correl_h(level,h,Corr);
        //Corr=cl->correl(level,h);
        Climat::Get_Correl_h_up(cl,level,h,Corr);
        ClimatData::Pr Vcl0hn1;
        Climat::Get_Vcl_h(cl,h,Vcl0hn1);
        OutData::V delV;
        delV.VZ=V0hn1.VZ-Vcl0hn1.VZ;
        delV.VM=V0hn1.VM-Vcl0hn1.VM;
        Eps.EpsZ=pow(Si0200.SiVZ,2)+pow(Si0h.SiVZ,2)-2*Si0200.SiVZ*Si0h.SiVZ*Corr.CorrZ+pow(Eps0h.EpsZ,2)+pow(delV.VZ,2);
        Eps.EpsZ=pow(Eps.EpsZ,0.5);
        Eps.EpsM=pow(Si0200.SiVM,2)+pow(Si0h.SiVM,2)-2*Si0200.SiVM*Si0h.SiVM*Corr.CorrM+pow(Eps0h.EpsM,2)+pow(delV.VM,2);
        Eps.EpsM=pow(Eps.EpsM,0.5);
        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_1p::Get_Vsr_0_200_3v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl){
   try{
       //Приземный ветер
        OutData::V V0;
        FunctionsCalc::Get_VZ_VM(InData::V0_tau0,InData::TETA0_tau0,/*VZ0,VM0*/V0);
        OutData::V Vh1;
        FunctionsCalc::Get_VZ_VM( ProfRPV[0].speed, ProfRPV[0].azimut, Vh1);

        //(46)
        OutData::V V0h1t;
        V0h1t.VZ=(V0.VZ+Vh1.VZ)/2;
        V0h1t.VM=(V0.VM+Vh1.VM)/2;

        ClimatData::Pr Vcl0h1, Vcl0200;
        double h1= ProfRPV[0].height;
        Climat::Get_Vcl_h(cl,h1,Vcl0h1);
        //!pr Climat::Get_Vcl(Constants::level200,Climat::Zone,Vcl0200);
        Vcl0200=cl->pr(Constants::level200);

        //(47)
        ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VZ=V0h1t.VZ-Vcl0h1.VZ+Vcl0200.VZ;
        ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VM=V0h1t.VM-Vcl0h1.VM+Vcl0200.VM;

        Get_Eps_3v(cl,Constants::level200, h1, ParamsVdVsr_0_200m__3_1p::ComplMode[2].Eps);

//        std::cout<<"V0200 3s: "<<ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VZ<<"\t"
//                <<ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200.VM<<std::endl;
        return true;
    }
    catch(...){
        return false;
    }
}

//Получает оценки теоретических ошибок 3-его способа
//ф.48
bool ParamsVdVsr_0_200m__3_1p::Get_Eps_3v(const ClimatData* cl,double level, double h, Calc_error::Eps &Eps){
    try{
        ClimatData::Sigma Si0200;
        //!si Climat::Get_Si(level,Climat::Zone,Si0200);
        Si0200=cl->sigma(level);
        ClimatData::Sigma Si0h;
        Climat::Get_Si_h(cl,h,Si0h);
        ClimatData::Correl Corr;
//cl!!!        Corr=cl->corr(level,h);
        //Corr=cl->correl(level,h);
        Climat::Get_Correl_h_up(cl,level,h,Corr);
        Eps.EpsZ=pow(Si0200.SiVZ,2)+2*pow(Si0h.SiVZ,2)-2*Si0200.SiVZ*Si0h.SiVZ*Corr.CorrZ;
        Eps.EpsZ=pow(Eps.EpsZ,0.5);
        Eps.EpsM=pow(Si0200.SiVM,2)+2*pow(Si0h.SiVM,2)-2*Si0200.SiVM*Si0h.SiVM*Corr.CorrM;
        Eps.EpsM=pow(Eps.EpsM,0.5);
        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_1p::Get_Vsr_0_200_4v(OutData::V **VCast){
   try{
      //Если отсутствуют данные по предыдущим зондированиям
        if(!InData::prevZond)
          return false;
        double deltau[InData::kprev];
        int numMB=GetNumMB(VCast,deltau);
        if (numMB<=0)
          return false;
        ParamsVdVsr_0_200m__3_1p::ComplMode[3].V0200.VZ=VCast[numMB-1][0].VZ;//ф.52
        ParamsVdVsr_0_200m__3_1p::ComplMode[3].V0200.VM=VCast[numMB-1][0].VM;
        ParamsVdVsr_0_200m__3_1p::ComplMode[3].Eps=bottomEval__2p1.GetEps_f28(deltau[numMB-1]);

        std::cout<<"V0200 4s: "<<ParamsVdVsr_0_200m__3_1p::ComplMode[3].V0200.VZ<<"\t"
                <<ParamsVdVsr_0_200m__3_1p::ComplMode[3].V0200.VM<<std::endl;
      return true;
    }
    catch(...){
        return false;
    }
}

//Получае номер метеобюллетеня для корректировки (начиная с 1)
int ParamsVdVsr_0_200m__3_1p::GetNumMB(OutData::V **VCast,double *deltau ){
   //Номер метеобюллетеня для корректировки
   int numMB=-1;
   //double deltau[InData::kprev];
   double CH0=InData::GetCHfromDate(InData::tau0.GD0,InData::tau0.MS0,InData::tau0.DN0, InData::tau0.CH0);
   double CHM;
   //Поиск временных сдвигов м/у расчетным моментом времени и временем МБ
    for(int k=0;k<InData::kprev;k++){
       CHM=InData::GetCHfromDate(InData::MBulletin_table[k].tauM.GD0,InData::MBulletin_table[k].tauM.MS0,
                                 InData::MBulletin_table[k].tauM.DN0,InData::MBulletin_table[k].tauM.CH0);
       deltau[k]=CH0-CHM;
    }
    MathFunc::Rank R_deltau[InData::kprev];
    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
    //!!!Для проверки
    /*for(int k=0;k<InData::kprev;k++)
        std::cout<<"MB№ "<<R_deltau[k].ind<<"\t"<<"deltau="<<R_deltau[k].mean<<std::endl;*/
    for (int i=0;i<InData::kprev;i++){
        if (R_deltau[i].mean<=3){
          //Проверка 2-ого критерия
          if (abs(InData::MBulletin_table[R_deltau[i].ind].Hm-InData::H)<=Constants::level50){
           //Проверка 3-его критерия
            if(InData::MBulletin_table[R_deltau[i].ind].ProfMeteo[0].V!=Constants::UNDEF_MeteoB){
              //Проверка 4-ого критерия
              double V0200j, TETA0200j;
              FunctionsCalc::Get_VZ_VM_inv(VCast[R_deltau[i].ind][0],V0200j,TETA0200j);
              double Vmod=GetdelV_f49(V0200j,TETA0200j);
              double Vdop=bottomEval__2p1.GetdelVdop(R_deltau[i].mean);
              if (Vmod<=Vdop){
                  numMB=R_deltau[i].ind+1;//метеобюллетень найден
                  return numMB;
              }
            }

          }
       }
       else{
            if (i==0)
                return numMB;//ни один МБ не может быть использован
       }

    }

   return numMB;
}

double ParamsVdVsr_0_200m__3_1p::GetdelV_f49(double Vj, double TETAj){
   double delV=Constants::UNDEF_V;
   double V0200_t,TETA0200_t;
   GetVTETA0200(V0200_t, TETA0200_t);
   delV=bottomEval__2p1.GetdelV(V0200_t, TETA0200_t, Vj, TETAj);
   return delV;
}

void ParamsVdVsr_0_200m__3_1p::GetVTETA0200(double &V0200_t, double &TETA0200_t){
    V0200_t=Constants::UNDEF_V;
    TETA0200_t=Constants::UNDEF_TETA;
    OutData::V V0200_1=ParamsVdVsr_0_200m__3_1p::ComplMode[0].V0200;
    OutData::V V0200_2=ParamsVdVsr_0_200m__3_1p::ComplMode[1].V0200;
    OutData::V V0200_3=ParamsVdVsr_0_200m__3_1p::ComplMode[2].V0200;
    OutData::V V0200ZM_t;
    if((V0200_2.VZ!=Constants::UNDEF_V)&&(V0200_2.VM!=Constants::UNDEF_V)){//ф.50
        V0200ZM_t.VZ=(V0200_1.VZ+V0200_2.VZ+V0200_3.VZ)/3;
        V0200ZM_t.VM=(V0200_1.VM+V0200_2.VM+V0200_3.VM)/3;
    }
    else{//ф.51
        V0200ZM_t.VZ=(V0200_1.VZ+V0200_3.VZ)/2;
        V0200ZM_t.VM=(V0200_1.VM+V0200_3.VM)/2;
    }
    FunctionsCalc::Get_VZ_VM_inv(V0200ZM_t,V0200_t,TETA0200_t);
}



