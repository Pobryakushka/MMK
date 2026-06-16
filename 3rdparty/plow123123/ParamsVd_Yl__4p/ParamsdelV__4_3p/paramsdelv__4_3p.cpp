#include "paramsdelv__4_3p.h"
#include "InData/indataclimat.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVd_Yl__4p/paramsvd_yl__4p.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "BottomEval__2p/bottomeval__2p.h"
#include "FunctionsCalc/calc_bottomlayer.h"

int ParamsdelV__4_3p::IPRHH=-1;
ParamsdelV__4_3p::Mode ParamsdelV__4_3p::ComplMode[3];

ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p3;
BottomEval__2p bottomEval__2p3;

ParamsdelV__4_3p::ParamsdelV__4_3p()
{

}

bool ParamsdelV__4_3p::GetdelV(PlowAlgoritm::ProfilePoint* ProvRPV, const ClimatData* cl,int l, double Yl,double Hl,
                               OutData::V V0Hlsr,OutData::V **VCast,int &kmod) {
    try{
        kmod=0;
        Calc_error::Eps Eps1,Eps2,Eps3;
        ComplMode[0].delV0Yl0Hl=GetdelV_1v(cl,Yl, Hl,V0Hlsr, Eps1);
        ComplMode[0].Eps=Eps1;
        ComplMode[1].delV0Yl0Hl=GetdelV_2v(ProvRPV,cl,l, Yl, Hl, Eps2);
        ComplMode[1].Eps=Eps2;
        ComplMode[2].delV0Yl0Hl=GetdelV_3v(cl,/*l*/Yl, Hl,V0Hlsr, VCast, Eps3);
        ComplMode[2].Eps=Eps3;

for(int m=0;m<3;m++){
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VZ=ComplMode[m].delV0Yl0Hl.VZ;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].V0200.VM=ComplMode[m].delV0Yl0Hl.VM;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsZ=ComplMode[m].Eps.EpsZ;
    ParamsVdVsr_0_200m__3_1p::ComplMode[m].Eps.EpsM=ComplMode[m].Eps.EpsM;
}


        //!!!Для проверки
        std::cout<<"------p.4.3 delV------"<<std::endl;
        for(int i=0;i<3;i++)
            std::cout<<ComplMode[i].delV0Yl0Hl.VZ<<"\t"<<ComplMode[i].delV0Yl0Hl.VM
                 <<"\t"<<ComplMode[i].Eps.EpsZ<<"\t"<<ComplMode[i].Eps.EpsM<<std::endl;
        kmod=Getkmod();
        IPRHH=0;

        return true;
    }
    catch(...){
        return false;
    }
}

int ParamsdelV__4_3p::Getkmod(){
    int kmod=0;
    bool b1=((ComplMode[0].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&& (ComplMode[0].delV0Yl0Hl.VM!=Constants::UNDEF_V));
    bool b2=((ComplMode[1].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&& (ComplMode[1].delV0Yl0Hl.VM!=Constants::UNDEF_V));
    //в случае возможности расчета по 2-м вариантам
    if(b1&&b2){
        OutData::V delVsr;
        delVsr.VZ=(ComplMode[0].delV0Yl0Hl.VZ+ComplMode[1].delV0Yl0Hl.VZ)/2;//ф.121
        delVsr.VM=(ComplMode[0].delV0Yl0Hl.VM+ComplMode[1].delV0Yl0Hl.VM)/2;
        bool b3=((fabs(ComplMode[0].delV0Yl0Hl.VZ-delVsr.VZ))>(0.33*fabs(delVsr.VZ)));
        bool b4=((fabs(ComplMode[0].delV0Yl0Hl.VM-delVsr.VM))>(0.33*fabs(delVsr.VM)));
        if(b3||(b4)){//ф.122
            Mode mode0;
            ComplMode[0]=mode0;//?????//модель среднеклиматического приращения отбраковывается
        }
    }
    for (int i=0;i<3;i++)
    if ((ComplMode[i].delV0Yl0Hl.VZ!=Constants::UNDEF_V)&& (ComplMode[i].delV0Yl0Hl.VM!=Constants::UNDEF_V))
       kmod++;
    return kmod;
}

//118
OutData::V ParamsdelV__4_3p::GetdelV_1v(const ClimatData* cl, double Yl, double Hl,OutData::V V0Hlsr,Calc_error::Eps &Eps1){
   OutData::V delV0Yl0Hl;
   ClimatData::Pr V0Ylcl,V0Hlcl;
   Climat::Get_Vcl_h(cl,Yl,V0Ylcl);
   Climat::Get_Vcl_h(cl,Hl,V0Hlcl);
   delV0Yl0Hl.VZ=V0Ylcl.VZ-V0Hlcl.VZ;
   delV0Yl0Hl.VM=V0Ylcl.VM-V0Hlcl.VM;
   Eps1=GetEps1(cl,Yl,Hl,V0Hlsr,V0Hlcl);
   return delV0Yl0Hl;
}

Calc_error::Eps ParamsdelV__4_3p::GetEps1(const ClimatData* cl, double Yl,double Hl, OutData::V V0Hlsr,
                                          ClimatData::Pr V0Hlcl){
    Calc_error::Eps Eps1;
    ClimatData::Sigma Si0Yl,Si0Hl;
    //!si Climat::Get_Si(Yl,Climat::Zone,Si0Yl);
    //Si0Yl=cl->sigma(Yl);
    Climat::Get_Si_h(cl,Yl,Si0Yl);
    Climat::Get_Si_h(cl,Hl,Si0Hl);
    ClimatData::Correl CorrYlHl;
    //!corr Climat::Get_Correl_h(Yl,Hl,CorrYlHl);
    Climat::Get_Correl_h_up(cl,Yl,Hl,CorrYlHl);
    Eps1.EpsZ=pow(Si0Yl.SiVZ,2)+pow(Si0Hl.SiVZ,2)-2*Si0Yl.SiVZ*Si0Hl.SiVZ*CorrYlHl.CorrZ+pow((V0Hlsr.VZ-V0Hlcl.VZ),2);
    Eps1.EpsZ=pow(Eps1.EpsZ,0.5);
    Eps1.EpsM=pow(Si0Yl.SiVM,2)+pow(Si0Hl.SiVM,2)-2*Si0Yl.SiVM*Si0Hl.SiVM*CorrYlHl.CorrM+pow((V0Hlsr.VM-V0Hlcl.VM),2);
    Eps1.EpsM=pow(Eps1.EpsM,0.5);
    return Eps1;
}

OutData::V ParamsdelV__4_3p::GetdelV_2v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int l, double Yl, double Hl, Calc_error::Eps &Eps2){
   OutData::V delV0Yl0Hl;
   int m;//номер измерения локатора, при котором hm>Yl
   OutData::VLayer V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
   bool p=FunctionsCalc::Poisk_hm_ontop(ProfRPV,Yl,m);
   bool v2=p && ((V0Hl.Vd.VZ!=Constants::UNDEF_V)&& (V0Hl.Vd.VM!=Constants::UNDEF_V));
   if (!v2)
       return delV0Yl0Hl;
  // double hm= ProfRPV[m].height;
   if(ProfRPV[m].curr!=1) return delV0Yl0Hl;
   delV0Yl0Hl=GetdelV2_f120(ProfRPV,cl,Yl, Hl, m, V0Hl);//ф.120
   Eps2=Calc_error::Get_Eps_f102(cl,Yl,Hl,ProfRPV[m].height);
   return delV0Yl0Hl;
}

OutData::V ParamsdelV__4_3p::GetdelV2_f120(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, double Yl, double Hl, int m,
                                           OutData::VLayer V0Hl){
   OutData::V delV0Yl0Hl2;
   double hm=ProfRPV[m].height;
   double h00=0;
   if(InData::bottomLayer) h00=InData::h0;
   OutData::V Vhm;
   FunctionsCalc::Get_VZ_VM(ProfRPV[m].speed,ProfRPV[m].azimut,Vhm);
   //120
   delV0Yl0Hl2.VZ=((Vhm.VZ-V0Hl.Vd.VZ)*pow((Yl-Hl),2))/(2*(Yl-h00)*(hm-Hl))+
           ((Yl-Hl)*(V0Hl.Vd.VZ-V0Hl.Vsr.VZ))/(Yl-h00);
   delV0Yl0Hl2.VM=((Vhm.VM-V0Hl.Vd.VM)*pow((Yl-Hl),2))/(2*(Yl-h00)*(hm-Hl))+
           ((Yl-Hl)*(V0Hl.Vd.VM-V0Hl.Vsr.VM))/(Yl-h00);;
   ClimatData::Sigma Si;
   Climat::Get_Si_h(cl,hm,Si);
   return delV0Yl0Hl2;
}

OutData::V ParamsdelV__4_3p::GetdelV2_f122(PlowAlgoritm::ProfilePoint* ProvRPV, const ClimatData* cl,double Yl,
                                           double Hl, double hm,OutData::VLayer V0hm, Calc_error::Eps &Eps2){
   OutData::V delV0Yl0Hl2;
   ClimatData::Pr V0hmcl,V0Ylcl,V0Hlcl;
   Climat::Get_Vcl_h(cl,hm,V0hmcl);
   Climat::Get_Vcl_h(cl,Yl,V0Ylcl);
   Climat::Get_Vcl_h(cl,Hl,V0Hlcl);
   delV0Yl0Hl2.VZ=V0hm.Vsr.VZ-V0hmcl.VZ+V0Ylcl.VZ-V0Hlcl.VZ;//122
   delV0Yl0Hl2.VM=V0hm.Vsr.VM-V0hmcl.VM+V0Ylcl.VM-V0Hlcl.VM;

   ClimatData::Sigma Si;
   Climat::Get_Si_h(cl,hm,Si);
   Calc_error::Eps Eps0hm;
   Eps0hm=Calc_error::Get_Eps(Si,0,hm,ProvRPV);//102
   OutData::V V0hn1;//????????????
   paramsVdVsr_0_200m__3_1p3.Get_Eps_2v(cl,hm,Yl,Eps0hm,V0hn1,Eps2);//123,45

   return delV0Yl0Hl2;
}

OutData::V ParamsdelV__4_3p::GetdelV_3v(const ClimatData* cl, double Yl,/* int l,*/ double Hl, OutData::V V0Hlsr,
                                        OutData::V **VCast,Calc_error::Eps &Eps3){
   OutData::V delV0Yl0Hl;
   //Если отсутствуют предыдущие зондирования
   if (!InData::prevZond)
       return delV0Yl0Hl;
   GetNumMB_H__2_5p(cl,Yl,Hl,V0Hlsr,VCast,delV0Yl0Hl,Eps3) ;
   return delV0Yl0Hl;
}

//Определяет номер метеобюллетеня для восстановления параметров
//среднего ветра в слояях выше 0-200
//V0Hlsr-скорость среднего ветра в слое 0-Hl,
//рассчитанная в п.4.1
int ParamsdelV__4_3p::GetNumMB_H__2_5p(const ClimatData* cl, double Yl,double Hl/*,int l*/,OutData::V V0Hlsr,OutData::V **VCast,
                                       OutData::V &delV_3v, Calc_error::Eps &Eps3){
    int numMB_H=-1;
    double _V0Hlsr, TETA0Hlsr;
    double V0Hlj, TETA0Hlj;
    FunctionsCalc::Get_VZ_VM_inv(V0Hlsr,_V0Hlsr,TETA0Hlsr);
    double delV[InData::kprev];
    double deltau[InData::kprev];
    double CH0=InData::GetCHfromDate(InData::tau0.GD0,InData::tau0.MS0,InData::tau0.DN0, InData::tau0.CH0);
    double CHM;
    for(int k=0;k<InData::kprev;k++){
        deltau[k]=/*-1*/100;
        delV[k]=100;
    }
    //Поиск временных сдвигов м/у расчетным моментом времени и временем МБ
    bool b1=false, b2=false, b3=false;
    OutData::V V0Hltau[InData::kprev];
    OutData::V V0Yltau[InData::kprev];
    int m,mpl1,n,npl1;
    FunctionsCalc::PoiskBoundsLevelsMB(Hl,m,mpl1);
    FunctionsCalc::PoiskBoundsLevelsMB(Yl,n,npl1);
    for(int k=0;k<InData::kprev;k++){
         b1=((VCast[k][0].VZ!=Constants::UNDEF_V)&&(VCast[k][0].VM!=Constants::UNDEF_V));
         b2=((VCast[k][1].VZ!=Constants::UNDEF_V)&&(VCast[k][1].VM!=Constants::UNDEF_V));
         b3=((VCast[k][2].VZ!=Constants::UNDEF_V)&&(VCast[k][2].VM!=Constants::UNDEF_V));
         if(b1 && b2 && b3){
           CHM=InData::GetCHfromDate(InData::MBulletin_table[k].tauM.GD0,InData::MBulletin_table[k].tauM.MS0,
                                   InData::MBulletin_table[k].tauM.DN0,InData::MBulletin_table[k].tauM.CH0);
           deltau[k]=CH0-CHM;
           if(Hl<=Constants::level200)
               V0Hltau[k]=VCast[k][0];
           else{
             //  int m,mpl1;
              // FunctionsCalc::PoiskBoundsLevelsMB(Hl,m,mpl1);
               FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),m,mpl1,Hl,VCast,V0Hltau[k]);
           }
          FunctionsCalc::Get_VZ_VM_inv(V0Hltau[k],V0Hlj,TETA0Hlj);
          delV[k]=bottomEval__2p3.GetdelV(_V0Hlsr, TETA0Hlsr,V0Hlj,TETA0Hlj);//ф.124

          FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),n,npl1,Yl,VCast,V0Yltau[k]);

         }
      }

    MathFunc::Rank R_deltau[InData::kprev];
    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
    MathFunc::Rank R_delV[InData::kprev];
    MathFunc::RankingIncrMas1(delV,InData::kprev,R_delV);

    int M[2][InData::kprev];
    for (int k=0;k<InData::kprev;k++){//Заполняем массив мест
        M[0][k]=bottomEval__2p3.GetM(R_deltau,InData::kprev,k);
        M[1][k]=bottomEval__2p3.GetM(R_delV,InData::kprev,k);
    }
    double sum[InData::k];
    for (int k=0;k<InData::kprev;k++)
        sum[k]=M[0][k]+M[1][k];//ф.125
    MathFunc::Rank R_sum[InData::kprev];
    MathFunc::RankingIncrMas1(sum,InData::kprev,R_sum);
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
    for(int ll=1;ll<Constants::numStL_MB;ll++){
        undeff=((InData::MBulletin_table[numMB_H-1].ProfMeteo[ll].V==Constants::UNDEF_MeteoB)||
                (InData::MBulletin_table[numMB_H-1].ProfMeteo[ll].V==Constants::UNDEF_V));
        sumundeff=sumundeff && undeff;
    }
    if (sumundeff==true)
        return -1;
    if ((InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_MeteoB)||
            (InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_V))
        return -1;


    if((V0Yltau[numMB_H-1].VZ!=Constants::UNDEF_V) && (V0Yltau[numMB_H-1].VM!=Constants::UNDEF_V) &&
            (V0Hltau[numMB_H-1].VZ!=Constants::UNDEF_V) && (V0Hltau[numMB_H-1].VM!=Constants::UNDEF_V)){
    delV_3v.VZ=V0Yltau[numMB_H-1].VZ-V0Hltau[numMB_H-1].VZ;//ф.126
    delV_3v.VM=V0Yltau[numMB_H-1].VM-V0Hltau[numMB_H-1].VM;
    Eps3=bottomEval__2p3.Eps_2v(cl,Yl/*?????*/, Hl,deltau[numMB_H-1],V0Hlsr,V0Hltau[numMB_H-1]);//ф.127
    }
    return numMB_H;
}




////Определяет номер метеобюллетеня для восстановления параметров
////среднего ветра в слояях выше 0-200
////deltau-массив сдвигов по времени из п.2.3
////V0200corr-скорректированное/нескорр. значение скорости
//int ParamsdelV__4_3p::GetNumMB_H__2_5p(OutData::V V0200corr, double *deltau,OutData::V **VCast){
//    int numMB_H=-1;
//    double V0200corr_m, TETA0200;
//    FunctionsCalc::Get_VZ_VM_inv(V0200corr,V0200corr_m,TETA0200);
//    double delV[InData::kprev];
//    double Vj, TETAj;
//    double YN[InData::kprev];
//    for (int j=0;j<InData::kprev;j++){
//        FunctionsCalc::Get_VZ_VM_inv(VCast[j][0],Vj,TETAj);
//        if ((Vj!=Constants::UNDEF_V)&&(Vj!=Constants::UNDEF_MeteoB))
//          delV[j]=GetdelV(V0200corr_m,TETA0200,Vj,TETAj);
//        else delV[j]=Constants::UNDEF_V;
//        YN[j]=InData::MBulletin_table[j].Hd;
//    }
//    //---------------------
//    //!!!Для проверки
//    std::cout<<"NoRank:"<<std::endl;
//    std::cout<<deltau[0]<<"\t"<<deltau[1]<<"\t"<<deltau[2]<<"\t"<<deltau[3]<<std::endl;
//    std::cout<<delV[0]<<"\t"<<delV[1]<<"\t"<<delV[2]<<"\t"<<delV[3]<<std::endl;
//    std::cout<<YN[0]<<"\t"<<YN[1]<<"\t"<<YN[2]<<"\t"<<YN[3]<<std::endl;
//   //---------------------

//    MathFunc::Rank R_deltau[InData::kprev];
//    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
//    MathFunc::Rank R_delV[InData::kprev];
//    MathFunc::RankingIncrMas1(delV,InData::kprev,R_delV);
//    MathFunc::Rank R_YN[InData::kprev];
//    MathFunc::RankingDecrMas1(YN,InData::kprev,R_YN);
//    int M[3][InData::kprev];
//    for (int k=0;k<InData::kprev;k++){//Заполняем массив мест
//        M[0][k]=GetM(R_deltau,InData::kprev,k);
//        M[1][k]=GetM(R_delV,InData::kprev,k);
//        M[2][k]=GetM(R_YN,InData::kprev,k);
//    }
//    //!!!Для проверки
//    std::cout<<"Mesta:"<<std::endl;
//    for (int i=0;i<3;i++)
//        std::cout<<M[i][0]<<"\t"<<M[i][1]<<"\t"<<M[i][2]<<"\t"<<M[i][3]<<std::endl;


//    double sum[InData::k];
//    for (int k=0;k<InData::kprev;k++)
//        sum[k]=M[0][k]+M[1][k]+M[2][k];//ф.31
//    MathFunc::Rank R_sum[InData::kprev];
//    MathFunc::RankingIncrMas1(sum,InData::kprev,R_sum);
//    //------------
//    //!!!Для проверки
//    std::cout<<"NoRank sum:"<<std::endl;
//    std::cout<<sum[0]<<"\t"<<sum[1]<<"\t"<<sum[2]<<"\t"<<sum[3]<<"\t"<<std::endl;
//    std::cout<<"Rank sum:"<<std::endl;
//    std::cout<<R_sum[0].mean<<"\t"<<R_sum[1].mean<<"\t"<<R_sum[2].mean<<"\t"<<R_sum[3].mean<<"\t"<<std::endl;
//    std::cout<<"N MB:"<<std::endl;
//    std::cout<<R_sum[0].ind+1<<"\t"<<R_sum[1].ind+1<<"\t"<<R_sum[2].ind+1<<"\t"<<R_sum[3].ind+1<<"\t"<<std::endl;
//    //------------
//    int eqsum=0;//кол-во МБ с равными sumj
//    for (int k=0;k<(InData::kprev-1);k++){
//        if(R_sum[k].mean==R_sum[k+1].mean)
//           eqsum++;
//        else break;
//    }
//    numMB_H=R_sum[0].ind+1;
//    if (eqsum>0){
//      double mindeltau=/*R_deltau[0].mean*/deltau[R_sum[0].ind];
//      for(int k=1;k<=eqsum;k++)
//          if (deltau[R_sum[k].ind]<mindeltau){
//             numMB_H=R_sum[k].ind+1;
//             mindeltau=deltau[R_sum[k].ind];
//          }
//    }
//    bool undeff=false;
//    bool sumundeff=true;
//    for(int l=1;l<Constants::numStL;l++){
//        undeff=((InData::MBulletin_table[numMB_H-1].ProfMeteo[l].V==Constants::UNDEF_MeteoB)||
//                (InData::MBulletin_table[numMB_H-1].ProfMeteo[l].V==Constants::UNDEF_V));
//        sumundeff=sumundeff && undeff;
//    }
//    if (sumundeff==true)
//        return -1;
//    if ((InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_MeteoB)||
//            (InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_V))
//        return -1;
//    return numMB_H;
//}










