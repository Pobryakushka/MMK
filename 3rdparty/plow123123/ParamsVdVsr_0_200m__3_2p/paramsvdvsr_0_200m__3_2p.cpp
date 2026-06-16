#include "paramsvdvsr_0_200m__3_2p.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "OutData/outdata.h"
#include "InData/indataclimat.h"
#include "InData/Constants.h"
#include "BottomEval__2p/bottomeval__2p.h"
#include "FunctionsCalc/mathfunc.h"

ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p2;
BottomEval__2p bottomEval__2p2;

ParamsVdVsr_0_200m__3_2p::ParamsVdVsr_0_200m__3_2p()
{

}

bool ParamsVdVsr_0_200m__3_2p::Do_3_2(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,
                                      OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::V **VCast){
    try{
        this->ProfRPV = ProfRPV;
        this->cl=cl;
        //OutData::VLayer VZ0200,  VZ0100, VZ050;
        //Рассчитываем средний ветер в слое 0-200м
        Get_Vsr_0_200(V0200,  V0100,VCast);
        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_2p::Get_Vsr_0_200(OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::V **VCast){
    try{
        if (InData::k<=1)
            return false;
        OutData::V V0200_1;
        Calc_error::Eps Eps1;
        if (InData::k>=3){
           Method0h1(1, V0200_1,Eps1);
        }
        else if(InData::k>=2){
           V0200_1=GetV0200_1_2k(Eps1);
        }

        if (InData::prevZond){
            //!!!Предыдущие зондирования
            V0200.Vsr=GetV0200pZ(V0200_1,Eps1,VCast);
        }
        else
            V0200.Vsr=V0200_1;//81

        V0100.Vsr.VZ=V0200.Vsr.VZ*0.82;//61
        V0100.Vsr.VM=V0200.Vsr.VM*0.83;

        return true;
    }
    catch(...){
        return false;
    }
}

//Методика для расчета среднего ветра в слое 0-hn1 стр.88
bool ParamsVdVsr_0_200m__3_2p::Method0h1(int n1, OutData::V &V0200_1,Calc_error::Eps &Eps1){
   OutData::V *VZM=new OutData::V [InData::k];
   paramsVdVsr_0_200m__3_1p2.GetMasVZM(ProfRPV,VZM);
   int nk=0;
   std::list<int>::iterator it;
   FunctionsCalc::Gr gr[InData::k];
   list<int> MK=paramsVdVsr_0_200m__3_1p2.FormMK(n1, ProfRPV, VZM,gr,nk);
   if(nk==0){
       V0200_1=GetV0200_1(gr,Eps1);
       delete [] VZM;
       return true;
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
           V0hn1=paramsVdVsr_0_200m__3_1p2.GetV0hn1_1v(Vi, Vj, Vn1, hi, hj, hn1);
       if((hi>hn1) && (hj>hn1))
           V0hn1=paramsVdVsr_0_200m__3_1p2.GetV0hn1_2v(Vi, Vj, Vn1, hi, hj, hn1);
       //if((hi>hn1) && (hj<hn1) || (hi<hn1) && (hj>hn1))
       if(((hi>hn1) && (hj<hn1)) || ((hi<hn1) && (hj>hn1)))
           V0hn1=paramsVdVsr_0_200m__3_1p2.GetV0hn1_3v(Vi, Vj, Vn1, hi, hj);
       double hmas[3];
       hmas[0]=hi;hmas[1]=hj;hmas[2]=hn1;
       int ind;
       double hh=MathFunc::Poisk_maxd(hmas,3,ind);
       ClimatData::Sigma Si0hh;
       Calc_error::Eps Eps0hn1;
       Climat::Get_Si_h(cl, hh,Si0hh);
       Eps0hn1=Calc_error::Get_Eps(Si0hh,0,hh,ProfRPV);
       ClimatData::Pr pr0200=paramsVdVsr_0_200m__3_1p2.GetV0200_f43_43(cl,hn1, V0hn1);
       V0200[c].VZ=pr0200.VZ;
       V0200[c].VM=pr0200.VM;
       paramsVdVsr_0_200m__3_1p2.Get_Eps_2v(cl,Constants::level200, hn1, Eps0hn1, V0hn1, Eps[c]);

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
   V0200_1.VZ=V0200Z/nk;
   V0200_1.VM=V0200M/nk;
   Eps1.EpsZ=pow(epsZ/nk,0.5);
   Eps1.EpsM=pow(epsM/nk,0.5);

//      cout<<V0200_2.VZ<<"\t"<<V0200_2.VM<<endl;
//      cout<<Eps2.EpsZ<<"\t"<<Eps2.EpsM<<endl;
   delete [] VZM;
   return true;
}

OutData::V ParamsVdVsr_0_200m__3_2p::GetV0200_1(FunctionsCalc::Gr* gr, Calc_error::Eps &Eps1){
    OutData::V V0200_1;
    double h1=ProfRPV[0].height;
    double masgr[InData::k];
    double sum=0;
    for(int i=0;i<InData::k;i++){
        sum=pow(gr[i].Z,2)+ pow(gr[i].M,2);
        masgr[i]=pow(sum,0.5);
    }
    int j=0;
    MathFunc::Poisk_mind(masgr,InData::k,j);
    Calc_error::Eps Eps0h1;
    OutData::V V0h1=GetV0h1(0, j, Eps0h1);
    ClimatData::Pr pr0200=paramsVdVsr_0_200m__3_1p2.GetV0200_f43_43(cl,h1,V0h1);
    V0200_1.VZ=pr0200.VZ;
    V0200_1.VM=pr0200.VM;
    paramsVdVsr_0_200m__3_1p2.Get_Eps_2v(cl,Constants::level200, h1, Eps0h1, V0h1, Eps1);
    return V0200_1;
}

//Расчет V0200_1 для случая наличия еще только одного уровня (h2) кроме h1
OutData::V ParamsVdVsr_0_200m__3_2p::GetV0200_1_2k(Calc_error::Eps &Eps1){
    OutData::V V0200_1;
    Calc_error::Eps Eps0h1;
    OutData::V V0h1=GetV0h1(0, 1, Eps0h1);
    double h1=ProfRPV[0].height;
    ClimatData::Pr pr0200=paramsVdVsr_0_200m__3_1p2.GetV0200_f43_43(cl,h1,V0h1);
    V0200_1.VZ=pr0200.VZ;
    V0200_1.VM=pr0200.VM;
    paramsVdVsr_0_200m__3_1p2.Get_Eps_2v(cl,Constants::level200, h1, Eps0h1, V0h1, Eps1);
    return V0200_1;
}

//ф.80
OutData::V ParamsVdVsr_0_200m__3_2p::GetV0h1(int i, int j,Calc_error::Eps &Eps0h1){
   OutData::V V0h1;
   double hi=ProfRPV[i].height;
   double hj=ProfRPV[j].height;
   OutData::V Vi, Vj;
   FunctionsCalc::Get_VZ_VM(ProfRPV[i].speed,ProfRPV[i].azimut,Vi);
   FunctionsCalc::Get_VZ_VM(ProfRPV[j].speed,ProfRPV[j].azimut,Vj);
   ClimatData::Pr V0hicl, V0hjcl;
   Climat::Get_Vcl_h(cl,hi,V0hicl);
   Climat::Get_Vcl_h(cl,hj,V0hjcl);
   ClimatData::Pr delVjicl;
   delVjicl.VZ=V0hjcl.VZ-V0hicl.VZ;
   delVjicl.VM=V0hjcl.VM-V0hicl.VM;
   V0h1.VZ=(0.5*(Vi.VZ+Vj.VZ)*(hj-hi)-hj*delVjicl.VZ)/(hj-hi);//ф.80
   V0h1.VM=(0.5*(Vi.VM+Vj.VM)*(hj-hi)-hj*delVjicl.VM)/(hj-hi);
   ClimatData::Sigma Si0hj;
   Calc_error::Eps Eps0hj;
   Climat::Get_Si_h(cl, hj,Si0hj);
   Eps0h1=Calc_error::Get_Eps(Si0hj,0,hj,ProfRPV);
   return V0h1;
}

//Расчет средней скорости в слое 0-200
//с учетом предыдущих зондирований
OutData::V ParamsVdVsr_0_200m__3_2p::GetV0200pZ(OutData::V V0200_1,Calc_error::Eps Eps1,OutData::V **VCast){
    OutData::V V0200sr;
    double deltau[InData::kprev];
    int numMB=GetNumMB(VCast,deltau,V0200_1);
    if (numMB<=0) V0200sr=V0200_1;//81
    else{
       OutData::V V0200_2=VCast[numMB-1][0];
       Calc_error::Eps Eps2=bottomEval__2p2.GetEps_f28(deltau[numMB-1]);
       V0200sr=bottomEval__2p2.GetV0200_f30(V0200_1,V0200_2,Eps1,Eps2);
    }
    return V0200sr;
}

//Получае номер метеобюллетеня для корректировки (начиная с 1)
int ParamsVdVsr_0_200m__3_2p::GetNumMB(OutData::V **VCast,double *deltau,OutData::V V0200_1 ){
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
              double Vmod=GetdelV_f82(V0200j,TETA0200j,V0200_1);
              double Vdop=bottomEval__2p2.GetdelVdop(R_deltau[i].mean);
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

//ф.82
double ParamsVdVsr_0_200m__3_2p::GetdelV_f82(double Vj, double TETAj,OutData::V V0200_1){
   double delV=Constants::UNDEF_V;
   double _V0200_1,_TETA0200_1;
   FunctionsCalc::Get_VZ_VM_inv(V0200_1,_V0200_1,_TETA0200_1);
   delV=bottomEval__2p2.GetdelV(_V0200_1, _TETA0200_1, Vj, TETAj);
   return delV;
}







