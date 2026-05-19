#include "bottomeval__2p.h"
#include "OutData/outdata.h"
#include "FunctionsCalc/calc_bottomlayer.h"
#include "FunctionsCalc/functionscalc.h"
#include "InData/InData.h"
#include "InData/indataclimat.h"
#include "FunctionsCalc/mathfunc.h"
#include "FunctionsCalc/calc_error.h"
//#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "climatdata.h"

#include <iostream>
#include "stdlib.h"
#include "stdio.h"
#include "math.h"

BottomEval__2p::BottomEval__2p()
{

}

void BottomEval__2p::GetVsr__2p(const ClimatData* cl, OutData::V *VsrYl, OutData::V **VCast){
    double V0200sr, TETA0200sr;
    Calc_bottomLayer::Get_V0_0_200(V0200sr,TETA0200sr);
    FunctionsCalc::Get_VZ_VM(V0200sr,TETA0200sr,VsrYl[1]);//ф.21
    //Если отсутсвуют данные предыдущих зондирований
    if(!InData::prevZond)
    {
        GetVsr__2_2p(cl,VsrYl);
        return;//Переход к блоку 6
    }
    //Если имеется массив предыдущих зондирований
    double deltau[InData::kprev];
    int numMB=GetNumMB__2_3p(V0200sr,TETA0200sr,deltau,VCast);
    if (numMB>0)//Корректировка скорости в слое 0-200
        VsrYl[1]=GetV0200__2_4p(cl,VsrYl[1],numMB,deltau[numMB-1]) ;//Переход к блоку 2.4;
    //Переход к блоку 2.5
    int numMB_H=GetNumMB_H__2_5p(VsrYl[1], deltau,VCast);
    //Переход к блоку 2.6
    GetVsr_H__2_6(cl,numMB_H,deltau[numMB_H-1],VCast,VsrYl);
}

void BottomEval__2p::GetVsr__2_2p(const ClimatData* cl, OutData::V *VsrYl){
    ClimatData::Pr Vcl0Y1;
    //!pr Climat::Get_Vcl(Climat::StandartLayer[0],Climat::Zone,Vcl0Y1);
    Vcl0Y1=cl->pr(Climat::StandartLayer[/*0*/1]);
    OutData::V V0Y1=VsrYl[/*0*/1];//на 200м ?????
    ClimatData::Pr Vcl0Yl;
    //ф.22
    for (int l=/*1*/2;l<Climat::numStL;l++){
       //!pr Climat::Get_Vcl(Climat::StandartLayer[l],Climat::Zone,Vcl0Yl);
       Vcl0Yl=cl->pr(Climat::StandartLayer[l]);
       VsrYl[l].VZ=V0Y1.VZ+Vcl0Yl.VZ-Vcl0Y1.VZ;
       VsrYl[l].VM=V0Y1.VM+Vcl0Yl.VM-Vcl0Y1.VM;
       std::cout<<"VsrYl"<<"\t"<<VsrYl[l].VZ<<"\t"<<VsrYl[l].VM<<std::endl;
   }
}

//Получае номер метеобюллетеня для корректировки (начиная с 1)
int BottomEval__2p::GetNumMB__2_3p(/*OutData::V *VsrYl,*/double V0200sr, double TETA0200sr,double *deltau,OutData::V **VCast ){
   //Номер метеобюллетеня для корректировки
   int numMB=-1;
  // double deltau[InData::kprev];
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
              double Vmod=GetdelV(V0200sr, TETA0200sr,V0200j,TETA0200j);
              double Vdop=GetdelVdop(R_deltau[i].mean);
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

//ф.26
//Получает оценку средней временной изменчивости
//ветра в зависимости от сдвига по времени
//deltau - рассчитанный по ф.23 временной сдвиг
//с рассматриваемым метеобюллетенем
double BottomEval__2p::GetdelVdop(double deltau){
    return Constants::k0[/*0*/1]*pow(deltau,0.5);
}

//ф.24
//Получает модуль вектора сдвига ветра
//V0, Vj - модули векторов скорости
//TETA0, TETAj - углы направления соответствующих векторов
double BottomEval__2p::GetdelV(double V0, double TETA0, double Vj, double TETAj){
    double delV=Constants::UNDEF_V;
    try{
      TETA0=TETA0*M_PI/180;
      TETAj=TETAj*M_PI/180;
      delV=pow(V0,2)+pow(Vj,2)-2*V0*Vj*cos(TETA0-TETAj);
      delV=pow(delV,0.5);
      return delV;
    }
    catch(...){
      return delV;
    }
}

//Получает скорректированную скорость среднего ветра в слое 0-200
//V02001-скорость, рассчитанная по данным о приземном ветре
//numMB-номер метеобюллетеня, взятого для коррекции
//deltau-временной сдвиг м/у расчетным моментом и временем МБ № numMB
OutData::V BottomEval__2p::GetV0200__2_4p(const ClimatData*cl, OutData::V V02001, int numMB,double deltau){
   OutData::V V0200;
   OutData::V V02002;
   FunctionsCalc::Get_VZ_VM(InData::MBulletin_table[numMB-1].ProfMeteo[0].V,
           InData::MBulletin_table[numMB-1].ProfMeteo[0].TETA,V02002);
   Calc_error::Eps Eps1,Eps2;
   ClimatData::Sigma Si0200;
   //!si Climat::Get_Si(Constants::level200,Climat::Zone,Si0200);
   Si0200=cl->sigma(Constants::level200);
   Eps1.EpsZ=Si0200.SiVZ;//ф.27
   Eps1.EpsM=Si0200.SiVM;
   //Eps2.EpsZ=Eps2.EpsM=Constants::k0[0]*pow(deltau,0.5);//ф.28
   Eps2=GetEps_f28(deltau);
   V0200=GetV0200_f30(V02001, V02002, Eps1, Eps2);
   return V0200;
}

OutData::V BottomEval__2p::GetV0200_f30(OutData::V V02001, OutData::V V02002, Calc_error::Eps Eps1,Calc_error::Eps Eps2){
    OutData::V V0200;
    double PZ[2],PM[2];
    GetPZM(Eps1.EpsZ, Eps2.EpsZ, PZ);//ф.29
    GetPZM(Eps1.EpsM, Eps2.EpsM, PM);
    V0200.VZ=PZ[0]*V02001.VZ+PZ[1]*V02002.VZ;//ф.30
    V0200.VM=PM[0]*V02001.VM+PM[1]*V02002.VM;
    return V0200;
}

Calc_error::Eps BottomEval__2p::GetEps_f28(double deltau){
  Calc_error::Eps eps;
  eps.EpsZ=eps.EpsM=Constants::k0[/*0*/1]*pow(deltau,0.5);//ф.28
  return eps;
}

//Расчет вероятностей по ф.29
void BottomEval__2p::GetPZM(double EpsZM1, double EpsZM2, double *P){
    if (EpsZM1==EpsZM2)
        P[0]=P[1]=0.5;
    if (EpsZM1<EpsZM2){
        P[0]=0.67;
        P[1]=0.33;
    }
    if (EpsZM1>EpsZM2){
        P[0]=0.33;
        P[1]=0.67;
    }
}

//Определяет номер метеобюллетеня для восстановления параметров
//среднего ветра в слояях выше 0-200
//deltau-массив сдвигов по времени из п.2.3
//V0200corr-скорректированное/нескорр. значение скорости
int BottomEval__2p::GetNumMB_H__2_5p(OutData::V V0200corr, double *deltau,OutData::V **VCast){
    int numMB_H=-1;
    double V0200corr_m, TETA0200;
    FunctionsCalc::Get_VZ_VM_inv(V0200corr,V0200corr_m,TETA0200);
    double delV[InData::kprev];
    double Vj, TETAj;
    double YN[InData::kprev];
    for (int j=0;j<InData::kprev;j++){
        FunctionsCalc::Get_VZ_VM_inv(VCast[j][0],Vj,TETAj);
        if ((Vj!=Constants::UNDEF_V)&&(Vj!=Constants::UNDEF_MeteoB))
          delV[j]=GetdelV(V0200corr_m,TETA0200,Vj,TETAj);
        else delV[j]=Constants::UNDEF_V;
        YN[j]=InData::MBulletin_table[j].Hd;
    }
    //---------------------
    //!!!Для проверки
    std::cout<<"NoRank:"<<std::endl;
    std::cout<<deltau[0]<<"\t"<<deltau[1]<<"\t"<<deltau[2]<<"\t"<<deltau[3]<<std::endl;
    std::cout<<delV[0]<<"\t"<<delV[1]<<"\t"<<delV[2]<<"\t"<<delV[3]<<std::endl;
    std::cout<<YN[0]<<"\t"<<YN[1]<<"\t"<<YN[2]<<"\t"<<YN[3]<<std::endl;
   //---------------------

    MathFunc::Rank R_deltau[InData::kprev];
    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
    MathFunc::Rank R_delV[InData::kprev];
    MathFunc::RankingIncrMas1(delV,InData::kprev,R_delV);
    MathFunc::Rank R_YN[InData::kprev];
    MathFunc::RankingDecrMas1(YN,InData::kprev,R_YN);
    int M[3][InData::kprev];
    for (int k=0;k<InData::kprev;k++){//Заполняем массив мест
        M[0][k]=GetM(R_deltau,InData::kprev,k);
        M[1][k]=GetM(R_delV,InData::kprev,k);
        M[2][k]=GetM(R_YN,InData::kprev,k);
    }
    //!!!Для проверки
    std::cout<<"Mesta:"<<std::endl;
    for (int i=0;i<3;i++)
        std::cout<<M[i][0]<<"\t"<<M[i][1]<<"\t"<<M[i][2]<<"\t"<<M[i][3]<<std::endl;


    double sum[InData::k];
    for (int k=0;k<InData::kprev;k++)
        sum[k]=M[0][k]+M[1][k]+M[2][k];//ф.31
    MathFunc::Rank R_sum[InData::kprev];
    MathFunc::RankingIncrMas1(sum,InData::kprev,R_sum);
    //------------
    //!!!Для проверки
    std::cout<<"NoRank sum:"<<std::endl;
    std::cout<<sum[0]<<"\t"<<sum[1]<<"\t"<<sum[2]<<"\t"<<sum[3]<<"\t"<<std::endl;
    std::cout<<"Rank sum:"<<std::endl;
    std::cout<<R_sum[0].mean<<"\t"<<R_sum[1].mean<<"\t"<<R_sum[2].mean<<"\t"<<R_sum[3].mean<<"\t"<<std::endl;
    std::cout<<"N MB:"<<std::endl;
    std::cout<<R_sum[0].ind+1<<"\t"<<R_sum[1].ind+1<<"\t"<<R_sum[2].ind+1<<"\t"<<R_sum[3].ind+1<<"\t"<<std::endl;
    //------------
    int eqsum=0;//кол-во МБ с равными sumj
    for (int k=0;k<(InData::kprev-1);k++){
        if(R_sum[k].mean==R_sum[k+1].mean)
           eqsum++;
        else break;
    }
    numMB_H=R_sum[0].ind+1;
    if (eqsum>0){
      double mindeltau=/*R_deltau[0].mean*/deltau[R_sum[0].ind];
      for(int k=1;k<=eqsum;k++)
          if (deltau[R_sum[k].ind]<mindeltau){
             numMB_H=R_sum[k].ind+1;
             mindeltau=deltau[R_sum[k].ind];
          }
    }
    bool undeff=false;
    bool sumundeff=true;
    for(int l=1;l</*Constants::numStL*/Climat::numStL;l++){
        undeff=((InData::MBulletin_table[numMB_H-1].ProfMeteo[l].V==Constants::UNDEF_MeteoB)||
                (InData::MBulletin_table[numMB_H-1].ProfMeteo[l].V==Constants::UNDEF_V));
        sumundeff=sumundeff && undeff;
    }
    if (sumundeff==true)
        return -1;
    if ((InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_MeteoB)||
            (InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_V))
        return -1;
    return numMB_H;
}

//Определяет место МБ № ind по параметру Rankmas из числа k бюллетеней
int BottomEval__2p::GetM(MathFunc::Rank *Rankmas,int k,int ind){
  //  int mmax=0;
    for (int i=0;i<k;i++)
      if (Rankmas[i].ind==ind){
          if (i>0){
           int j=i;
              while(Rankmas[i].mean==Rankmas[j-1].mean){
              j--;
           }
             return (j+1);
          }
          return (i+1);
      }
  return -1;
}

void BottomEval__2p::GetVsr_H__2_6(const ClimatData* cl,int numMB_H,double deltauj,OutData::V **VCast,OutData::V *VsrYl){
    OutData::V delV0Yl0200_t;
    for(int l=3;l<=Climat::numStL;l++){
       delV0Yl0200_t=GetDelV0Yl0200_t(cl,l, numMB_H, deltauj,VsrYl[1],VCast);
       VsrYl[l-1].VZ=VsrYl[1].VZ+delV0Yl0200_t.VZ;//ф.32
       VsrYl[l-1].VM=VsrYl[1].VM+delV0Yl0200_t.VM;
   }
}

OutData::V BottomEval__2p::GetDelV0Yl0200_t(const ClimatData* cl,int l, int numMB_H, double deltauj, OutData::V V0200,OutData::V **VCast){
    OutData::V delV0Yl0200_t;
    Calc_error::Eps Eps1, Eps2;
    double Yl=Climat::StandartLayer[l-1];
    int iMB;
    bool b1;
    if (numMB_H>0){
        b1=false;
        if((Yl==600)||(Yl==2600)){
         if(Yl==600) b1=(VCast[numMB_H-1][1].VZ!=Constants::UNDEF_V)&&
           (VCast[numMB_H-1][1].VM!=Constants::UNDEF_V) &&
           (VCast[numMB_H-1][2].VZ!=Constants::UNDEF_V) && (VCast[numMB_H-1][2].VM!=Constants::UNDEF_V);
         if(Yl==2600) b1=(VCast[numMB_H-1][6].VZ!=Constants::UNDEF_V)&&
           (VCast[numMB_H-1][6].VM!=Constants::UNDEF_V) &&
           (VCast[numMB_H-1][7].VZ!=Constants::UNDEF_V) && (VCast[numMB_H-1][7].VM!=Constants::UNDEF_V);
        }
        else{
            iMB=Climat::Get_is_MB(Yl);
            b1=(VCast[numMB_H-1][iMB].VZ!=Constants::UNDEF_V)&&(VCast[numMB_H-1][iMB].VM!=Constants::UNDEF_V);
        }
       if(b1){
         OutData::V delV0Yl0200_t_1=delV0Yl0200_t_1v(cl,l,V0200,Eps1);
         OutData::V delV0Yl0200_t_2=delV0Yl0200_t_2v(cl,l, numMB_H, deltauj,VCast,V0200, Eps2);
         double PZ[2],PM[2];
         GetPZM(Eps1.EpsZ, Eps2.EpsZ, PZ);//ф.29
         GetPZM(Eps1.EpsM, Eps2.EpsM, PM);
         delV0Yl0200_t.VZ=PZ[0]*delV0Yl0200_t_1.VZ+PZ[1]*delV0Yl0200_t_2.VZ;//ф.37
         delV0Yl0200_t.VM=PM[0]*delV0Yl0200_t_1.VM+PM[1]*delV0Yl0200_t_2.VM;
         return delV0Yl0200_t;
       }

    }
    //Если нельзя использовать данные предыдущих зондирований
    //или для слоя l в МБ № numB_H отсутствуют данные о ветре
    delV0Yl0200_t=delV0Yl0200_t_1v(cl,l,V0200,Eps1);
    return delV0Yl0200_t;
}

//ф.33
OutData::V BottomEval__2p::delV0Yl0200_t_1v(const ClimatData* cl, int l,OutData::V V0200, Calc_error::Eps &Eps1){
  OutData::V delV0Yl0200_t;
  double Yl=Climat::StandartLayer[l-1];
  ClimatData::Pr V0Ylcl,V0200cl;
  V0Ylcl=cl->pr(Yl);
  V0200cl=cl->pr(Constants::level200);
  delV0Yl0200_t.VZ=V0Ylcl.VZ-V0200cl.VZ;
  delV0Yl0200_t.VM=V0Ylcl.VM-V0200cl.VM;
  Eps1=Eps_1v(cl,Yl,V0200);
  return delV0Yl0200_t;
}

//ф.34
Calc_error::Eps BottomEval__2p::Eps_1v(const ClimatData* cl, double Yl,OutData::V V0200){
   Calc_error::Eps Eps1;
   ClimatData::Sigma Si0Yl,Si0200;
   Si0Yl=cl->sigma(Yl);
   Si0200=cl->sigma(Constants::level200);
   ClimatData::Correl CorrYl200;
   CorrYl200=cl->correl(Yl,Constants::level200);
   ClimatData::Pr V0200cl=cl->pr(Constants::level200);
   Eps1.EpsZ=pow(Si0Yl.SiVZ,2)+pow(Si0200.SiVZ,2)-2*Si0Yl.SiVZ*Si0200.SiVZ*CorrYl200.CorrZ+pow((V0200.VZ-V0200cl.VZ),2);
   Eps1.EpsZ=pow(Eps1.EpsZ,0.5);
   Eps1.EpsM=pow(Si0Yl.SiVM,2)+pow(Si0200.SiVM,2)-2*Si0Yl.SiVM*Si0200.SiVM*CorrYl200.CorrM+pow((V0200.VM-V0200cl.VM),2);
   Eps1.EpsM=pow(Eps1.EpsM,0.5);
   return Eps1;
}

//ф.35
OutData::V BottomEval__2p::delV0Yl0200_t_2v(const ClimatData* cl,int l, int numMB_H, double deltauj, OutData::V **VCast,OutData::V _V0200, Calc_error::Eps &Eps2){
   OutData::V delV0Yl0200_t;
   double V0200,TETA200;
   OutData::V V0Ylj, V0200j;
   double Yl=Climat::StandartLayer[l-1];
   V0200=InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V;
   TETA200=InData::MBulletin_table[numMB_H-1].ProfMeteo[0].TETA;
   int i=0, ipl1=0;
   FunctionsCalc::PoiskBoundsLevelsMB(Climat::StandartLayer[l-1],i,ipl1);
   FunctionsCalc::GetVTeta_LineInterpol2_MB(numMB_H,i,ipl1,Yl,VCast,V0Ylj);
   FunctionsCalc::Get_VZ_VM(V0200,TETA200,V0200j);
   delV0Yl0200_t.VZ=V0Ylj.VZ-V0200j.VZ;
   delV0Yl0200_t.VM=V0Ylj.VM-V0200j.VM;

   Eps2=Eps_2v(cl,/*l*/Yl, Constants::level200,deltauj,_V0200,V0200j);
   return delV0Yl0200_t;
}

////ф.36
//Calc_error::Eps BottomEval__2p::Eps_2v(const ClimatData* cl, int l,double h, double deltauj,OutData::V _V0200,OutData::V V0200j){
//    Calc_error::Eps Eps2;
//    int ih=-1;
//    if(h<=Constants::level200) ih=0;
//    else ih=FunctionsCalc::Poisk_imsh_low(h);

//    double k0h=Constants::k0[ih];
//    double k0Yl=Constants::k0[l-1];
//    double Yl=Climat::StandartLayer[l-1];
//    ClimatData::Correl CorrYl200;
//   //!corr Climat::Get_Correl_h(Yl,Climat::StandartLayer[ih],CorrYl200);
//   //??? Climat::Get_Correl_h_low(cl,Yl,Climat::StandartLayer[ih])
//    CorrYl200=cl->correl(Yl,Climat::StandartLayer[ih]);
//    Eps2.EpsZ=deltauj*(pow(k0Yl,2)+pow(k0h,2)-2*k0Yl*k0h*CorrYl200.CorrZ)+pow((_V0200.VZ-V0200j.VZ),2);
//    Eps2.EpsZ=pow(Eps2.EpsZ,0.5);
//    Eps2.EpsM=deltauj*(pow(k0Yl,2)+pow(k0h,2)-2*k0Yl*k0h*CorrYl200.CorrM)+pow((_V0200.VM-V0200j.VM),2);
//    Eps2.EpsM=pow(Eps2.EpsM,0.5);
//    return Eps2;
//}

//ф.36
Calc_error::Eps BottomEval__2p::Eps_2v(const ClimatData* cl, /*int l*/double _Yl, double h, double deltauj,OutData::V _V0200,OutData::V V0200j){
    Calc_error::Eps Eps2;
    int ih=-1, iYl=-1;
    if(h<=Constants::level200) ih=/*0*/1;
     else ih=FunctionsCalc::Poisk_imsh_low(h);
    iYl=FunctionsCalc::Poisk_imsh_low(_Yl);

    double k0h=Constants::k0[ih];
    double k0Yl=Constants::k0[iYl];
    double Yl=Climat::StandartLayer[iYl];
    ClimatData::Correl CorrYl200;
   //! corr Climat::Get_Correl_h(Yl,Climat::StandartLayer[ih],CorrYl200);
    Climat::Get_Correl_h_up(cl,_Yl,Climat::StandartLayer[ih],CorrYl200);
    //CorrYl200=cl->correl(Yl,Climat::StandartLayer[ih]);
    Eps2.EpsZ=deltauj*(pow(k0Yl,2)+pow(k0h,2)-2*k0Yl*k0h*CorrYl200.CorrZ)+pow((_V0200.VZ-V0200j.VZ),2);
    Eps2.EpsZ=pow(Eps2.EpsZ,0.5);
    Eps2.EpsM=deltauj*(pow(k0Yl,2)+pow(k0h,2)-2*k0Yl*k0h*CorrYl200.CorrM)+pow((_V0200.VM-V0200j.VM),2);
    Eps2.EpsM=pow(Eps2.EpsM,0.5);
    return Eps2;
}







