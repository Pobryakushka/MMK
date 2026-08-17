//------------------------------------------------------------
//ParamsVdVsr_0_200m__3_4p
//
// Модуль реализует класс, реализующий расчеты параметров
// действительного и спеднего ветра в пределах слоя 0 - 200м
// (согласно пункту алгоритма 3.4).
//------------------------------------------------------------

#include <iostream>

#include "paramsvdvsr_0_200m__3_4p.h"
#include "ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "InData/InData.h"

#include "FunctionsCalc/functionscalc_layer.h"
#include "InData/Constants.h"
#include "ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.h"

ParamsVdVsr_0_200m__3_3p paramsvdvsr_0_200m__3_3p;
ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p1;
ParamsVdVsr_0_200m__3_2p paramsVdVsr_0_200m__3_2p2;

ParamsVdVsr_0_200m__3_4p::ParamsVdVsr_0_200m__3_4p()
{

}

bool ParamsVdVsr_0_200m__3_4p::Do_3_4(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, OutData::VLayer &V0200, OutData::VLayer &V0100,OutData::VLayer &V050,
                                      OutData::VLayer &V075, OutData::VLayer &V0150,OutData::V **VCast) {
    try{
        //this->ProfRPV = ProfRPV;
       // this->cl=cl;
        //Рассчитываем средний ветер в слое 0-200м
        Get_V_0_200(ProfRPV,cl,V0200, V0100,V050,V075, V0150, VCast);
        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_4p::Get_V_0_200(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,OutData::VLayer &V0200, OutData::VLayer &V0100,
                                           OutData::VLayer &V050, OutData::VLayer &V075,OutData::VLayer &V0150, OutData::V **VCast){
    try{
         Calc_error::Eps Eps1;
         OutData::VLayer V0200_1;
        //Имеется в наличии только 1 уровень измерений
            if (InData::k <= 1)
              return false;//завершение вычислений по п.3.4

            //Определяем параметры действительного ветра на уровне 100м
            GetParams_100m(ProfRPV,V0100.Vd);

            OutData::VLayer V0hm;
           // Calc_error::Eps Eps0hm;
            int i_m;
            FunctionsCalc::Poisk_hm(ProfRPV,Constants::level200,i_m);
            double hm=ProfRPV[i_m].height;
            FunctionsCalc::Get_VZ_VM(ProfRPV[i_m].speed,ProfRPV[i_m].azimut,V0hm.Vd);
            V0hm.Vsr=GetV0hm(ProfRPV,cl,i_m,V0100.Vd);
            //Если один из уровней совпадает с уровнем 100м
            if((V0hm.Vsr.VZ==Constants::UNDEF_V)&&(V0hm.Vsr.VM==Constants::UNDEF_V)){
                V0200_1.Vsr.VZ=V0100.Vd.VZ;//ф.108
                V0200_1.Vsr.VM=V0100.Vd.VM;
                ClimatData::Sigma Si0200=cl->sigma(Constants::level200,-1);
                Eps1.EpsZ=Si0200.SiVZ;
                Eps1.EpsM=Si0200.SiVM;
            }
            else{
              //Если hm совпадает с 200м
              if (hm==Constants::level200){
                  V0200_1.Vd.VZ=V0hm.Vd.VZ;
                  V0200_1.Vd.VM=V0hm.Vd.VM;
                  V0200_1.Vsr.VZ=V0hm.Vsr.VZ;
                  V0200_1.Vsr.VM=V0hm.Vsr.VM;
              }
              else//Первый вариант оценок
                 GetParams_200m1(i_m,V0hm.Vsr,V0100.Vd,ProfRPV,cl, V0200_1,Eps1);
            }
             if (InData::prevZond){
                 //!!!!Второй вариант оценок
                 V0200.Vsr=paramsVdVsr_0_200m__3_2p2.GetV0200pZ(V0200_1.Vsr,Eps1,VCast);
             }
             else
               V0200.Vsr=V0200_1.Vsr;

             //Если скорость Vd уже определена по линейной интерполяции
             if (V0200_1.Vd.VZ!=Constants::UNDEF_V)
                V0200.Vd=V0200_1.Vd;
             else{//если параметры V0hm не определены
                 if (V0hm.Vsr.VZ==Constants::UNDEF_V){
                     OutData::VLayer V0100t;
                     V0100t.Vsr.VZ=V0200.Vsr.VZ*0.82;
                     V0100t.Vsr.VM=V0200.Vsr.VM*0.83;
                     V0100t.Vd=V0100.Vd;
                     V0200.Vd=GetV200_f113(Constants::level200,Constants::level100,V0200.Vsr,V0100t);
                 }
                 else
                     V0200.Vd=GetV200_f113(Constants::level200,hm,V0200.Vsr,V0hm);//114
             }
           // V0100.Vsr=GetV0100_f114(V0200,V0100.Vd);
             V0100.Vsr.VZ=V0200.Vsr.VZ*0.82;//ф.61
             V0100.Vsr.VM=V0200.Vsr.VM*0.83;
             V050=GetV050(ProfRPV,V0100);// рассчитываются только действительный ветер

            V075.Vd=GetV75(ProfRPV,V050.Vd,V0100.Vd);
            V0150.Vd=GetV150(ProfRPV,V0100.Vd,V0200.Vd);
         return true;
    }
    catch(...){
        return false;
    }
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV0hm(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,int i_m, OutData::V V100){
    OutData::V V0hm;
    Calc_error::Eps Eps0hm;
    if(InData::k>=3)
        Method0hm(ProfRPV,cl,(i_m+1),V100, V0hm, Eps0hm);
    if(InData::k==2){
        OutData::V *VZM=new OutData::V [InData::k];
        paramsVdVsr_0_200m__3_1p1.GetMasVZM(ProfRPV,VZM);
        FunctionsCalc::Gr gr[InData::k];
        for(int i=0;i<InData::k;i++)
         if(i!=i_m)
             gr[i]=FunctionsCalc::GetGr(VZM[i],VZM[i_m],ProfRPV[i].height,ProfRPV[i_m].height);
         else {
             gr[i].Z=999;//??????????
             gr[i].M=999;
         }
        V0hm=GetV0hm_1v(ProfRPV,cl,i_m,V100,gr,Eps0hm);
    }
    return V0hm;
}

//Методика для расчета среднего ветра в слое 0-hm стр.111
bool ParamsVdVsr_0_200m__3_4p::Method0hm(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,int n1, OutData::V V100, OutData::V &V0200_1,Calc_error::Eps &Eps1){
   OutData::V *VZM=new OutData::V [InData::k];
   paramsVdVsr_0_200m__3_1p1.GetMasVZM(ProfRPV,VZM);
   int nk=0;
   std::list<int>::iterator it;
   FunctionsCalc::Gr gr[InData::k];
   list<int> MK=paramsVdVsr_0_200m__3_1p1.FormMK(n1, ProfRPV, VZM,gr,nk);
    {
       gr[n1-1].Z=999;//??????????
       gr[n1-1].M=999;
   }
   if(nk==0){
       V0200_1=GetV0hm_1v(ProfRPV,cl,n1-1,V100,gr,Eps1);
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
           V0hn1=paramsVdVsr_0_200m__3_1p1.GetV0hn1_1v(Vi, Vj, Vn1, hi, hj, hn1);
       if((hi>hn1) && (hj>hn1))
           V0hn1=paramsVdVsr_0_200m__3_1p1.GetV0hn1_2v(Vi, Vj, Vn1, hi, hj, hn1);
      // if((hi>hn1) && (hj<hn1) || (hi<hn1) && (hj>hn1))
        if(((hi>hn1) && (hj<hn1)) || ((hi<hn1) && (hj>hn1)))
           V0hn1=paramsVdVsr_0_200m__3_1p1.GetV0hn1_3v(Vi, Vj, Vn1, hi, hj);
       V0200[c].VZ=V0hn1.VZ;
       V0200[c].VM=V0hn1.VM;
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
//   Eps1.EpsZ=pow(epsZ/nk,0.5);
//   Eps1.EpsM=pow(epsM/nk,0.5);

//      cout<<V0200_2.VZ<<"\t"<<V0200_2.VM<<endl;
//      cout<<Eps2.EpsZ<<"\t"<<Eps2.EpsM<<endl;
   delete [] VZM;
   return true;
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV0hm_1v(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,int im, OutData::V V100, FunctionsCalc::Gr* gr, Calc_error::Eps &Epsm){
    OutData::V V0hm;
    double masgr[InData::k];
    double sum=0;
    //!!!Для проверки
    std::cout<<"gr p3.4"<<std::endl;
    for(int i=0;i<InData::k;i++){
        sum=pow(gr[i].Z,2)+ pow(gr[i].M,2);
        masgr[i]=pow(sum,0.5);

        std::cout<<i<<"\t"<<gr[i].Z<<"\t"<<gr[i].M<<"\t"<<masgr[i]<<std::endl;
    }



    //-----------------
    int j=0;
    MathFunc::Poisk_mind(masgr,InData::k,j);
    double hj=ProfRPV[j].height;
    if(hj>Constants::level200)
       V0hm=GetV0hm_f107(ProfRPV,cl,im, j, Epsm);//ф.107
    else{
        double hm=ProfRPV[im].height;
        if((hm==Constants::level100)||(hj==Constants::level100))
            return V0hm;
        else{
            double hl=Constants::level100;
            OutData::V Vl=V100, Vj, Vm;
            FunctionsCalc::Get_VZ_VM(ProfRPV[j].speed,ProfRPV[j].azimut,Vj);
            FunctionsCalc::Get_VZ_VM(ProfRPV[im].speed,ProfRPV[im].azimut,Vm);
            if((hl<hm) && (hj<hm))
                V0hm=paramsVdVsr_0_200m__3_1p1.GetV0hn1_1v(Vl, Vj, Vm, hl, hj, hm);
            if((hl>hm) && (hj>hm))
                V0hm=paramsVdVsr_0_200m__3_1p1.GetV0hn1_2v(Vl, Vj, Vm, hl, hj, hm);
            //if((hl>hm) && (hj<hm) || (hl<hm) && (hj>hm))
            if(((hl>hm) && (hj<hm)) || ((hl<hm) && (hj>hm)))
                V0hm=paramsVdVsr_0_200m__3_1p1.GetV0hn1_3v(Vl, Vj, Vm, hl, hj/*, hm*/);
        }

    }
    return V0hm;
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV0hm_f107(PlowAlgoritm::ProfilePoint * ProfRPV,const ClimatData* cl,int i, int j, Calc_error::Eps &Epsm){
    OutData::V V0hm;
    double hi=ProfRPV[i].height;
    double hj=ProfRPV[j].height;
    OutData::V Vi, Vj;
    FunctionsCalc::Get_VZ_VM(ProfRPV[i].speed,ProfRPV[i].azimut,Vi);
    FunctionsCalc::Get_VZ_VM(ProfRPV[j].speed,ProfRPV[j].azimut,Vj);
    ClimatData::Pr V0hicl, V0hjcl, V0200cl;
    Climat::Get_Vcl_h(cl,hj,V0hjcl);
    Climat::Get_Vcl_h(cl,Constants::level200,V0200cl);
    if(hi>=150) V0hicl=V0200cl;
      else{
        V0hicl.VZ=V0200cl.VZ*0.82;
        V0hicl.VM=V0200cl.VM*0.83;
    }
    ClimatData::Pr delVjicl;
    delVjicl.VZ=V0hjcl.VZ-V0hicl.VZ;
    delVjicl.VM=V0hjcl.VM-V0hicl.VM;
    V0hm.VZ=(0.5*(Vi.VZ+Vj.VZ)*(hj-hi)-hj*delVjicl.VZ)/(hj-hi);//ф.80
    V0hm.VM=(0.5*(Vi.VM+Vj.VM)*(hj-hi)-hj*delVjicl.VM)/(hj-hi);
    ClimatData::Sigma Si0hj;
    Calc_error::Eps Eps0hj;
    Climat::Get_Si_h(cl, hj,Si0hj);
    Epsm=Calc_error::Get_Eps(Si0hj,0,hj,ProfRPV);
    return V0hm;
}

OutData::VLayer ParamsVdVsr_0_200m__3_4p::GetV050(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::VLayer V0100){
    OutData::VLayer V050;
    int i, ipl1;
    if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level50,i,ipl1))
        FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,i,ipl1,Constants::level50,V050.Vd);
    else{
        double h1= ProfRPV[0].height;
        if(h1<=Constants::level100){
            double V50=FunctionsCalc::IndiffStrat(Constants::level50, ProfRPV[0].speed,h1);
            double TETA50= ProfRPV[0].azimut;
            FunctionsCalc::Get_VZ_VM(V50,TETA50,V050.Vd);
        }
        else{
            V050.Vd.VZ=Constants::UNDEF_V;
            V050.Vd.VM=Constants::UNDEF_V;
        }
    }

//    if ((V050.Vd.VZ!=Constants::UNDEF_V)&& (FunctionsCalc::INT100==1)){//не рассчитывается вообще
//        V050.Vsr.VZ=2*V0100.Vsr.VZ-(V050.Vd.VZ+V0100.Vd.VZ)/2;
//        V050.Vsr.VM=2*V0100.Vsr.VM-(V050.Vd.VM+V0100.Vd.VM)/2;
//    }
//    else{
//        V050.Vsr.VZ=Constants::UNDEF_V;
//        V050.Vsr.VM=Constants::UNDEF_V;
//    }
    return V050;
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV75(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V V50,OutData::V V100){
    OutData::V V75;
    int j,jpl1;
    if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level75,j,jpl1))
         FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level75,V75);//ф.94б
    else{
       bool b1=((V50.VZ!=Constants::UNDEF_V) && (V50.VM!=Constants::UNDEF_V));
       bool b2=(FunctionsCalc::INT100==1);
       if(b1 && b2)
          FunctionsCalc::GetVTeta_LineInterpol4(V50,V100,V75);//ф.79б
    }
    return V75;
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV150(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V V100, OutData::V V200){
    OutData::V V150;
    int j,jpl1;
    if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level150,j,jpl1))
         FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level150,V150);//ф.94б
    else{
       bool b2=(FunctionsCalc::INT100==1);
       if(b2)
          FunctionsCalc::GetVTeta_LineInterpol4(V100,V200,V150);//ф.79б
    }
    return V150;
}

OutData::V ParamsVdVsr_0_200m__3_4p::GetV0100_f114(OutData::VLayer V0200, OutData::V V100){
    OutData::V V0100;
    if (FunctionsCalc::INT100==1){
        V0100.VZ=2*V0200.Vsr.VZ-(V100.VZ+V0200.Vd.VZ)/2;
        V0100.VM=2*V0200.Vsr.VM-(V100.VM+V0200.Vd.VM)/2;
    }
    else{
        V0100.VZ=V0200.Vsr.VZ*0.82;
        V0100.VM=V0200.Vsr.VM*0.83;
    }
    return V0100;
}

//113
OutData::V ParamsVdVsr_0_200m__3_4p::GetV200_f113(double level, double hm, OutData::V V0200, OutData::VLayer V0hm){
    OutData::V V200;
    V200.VZ=2*(V0200.VZ*level-V0hm.Vsr.VZ*hm)/(level-hm)-V0hm.Vd.VZ;
    V200.VM=2*(V0200.VM*level-V0hm.Vsr.VM*hm)/(level-hm)-V0hm.Vd.VM;
    return V200;
}

bool ParamsVdVsr_0_200m__3_4p::Raspred(PlowAlgoritm::ProfilePoint* ProfRPV, double hm, double hj, int hl, int i_m, int i_j, int i_l,OutData::V V100d, OutData::VLayer &V0hm){
    try{
        //оба уровня ниже hm
        if ((hj<hm) && (hl<hm)) {
           FunctionsCalc_Layer::Get_VZ_VM_Layer_0_hm_1v(ProfRPV,hm, hj, hl, i_m ,i_j,i_l,V100d,V0hm);
        }
        //оба уровня выше hm
        if ((hj>hm) && (hl>hm)){
           FunctionsCalc_Layer::Get_VZ_VM_Layer_0_hm_2v(ProfRPV,hm, hj, hl, i_m ,i_j,i_l,V100d,V0hm);
        }
        //hm между уровнями
        if (((hm<hl) && (hm>hj)) || ((hm>hl) && (hm<hj))) {
            FunctionsCalc_Layer::Get_VZ_VM_Layer_0_hm_3v(ProfRPV, hj, hl, i_m ,i_j,i_l,V100d,V0hm);
        }
        return true;
    }
    catch(...){
        return false;
    }
}

//Получает параметры действительного ветра на уровне 100м
bool ParamsVdVsr_0_200m__3_4p::GetParams_100m(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V &V100)
{
  try{
    double Vh=0;
    double TETAh=0;
    int ind_i=0;
    int ind_i_pl_1=0;
    //Если найдены соседние уровни, между которыми находится уровень 100м
    if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level100,ind_i,ind_i_pl_1)&&
            (FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,ind_i,ind_i_pl_1))){
          FunctionsCalc::INT100=1;
          FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,ind_i, ind_i_pl_1, Constants::level100, V100);
    }
    else{
        FunctionsCalc::INT100=0;
        FunctionsCalc::GetVTeta_IndiffStrat(ProfRPV,Constants::level100, Vh, TETAh);
        FunctionsCalc::Get_VZ_VM(Vh,TETAh,V100);
    }
    return true;
   }
    catch(...)
    {
        return false;
    }
}

//Получает параметры действительного ветра на уровне 200м
bool ParamsVdVsr_0_200m__3_4p::GetParams_200m1(int i_m,OutData::V V0hmsr,OutData::V V100d,PlowAlgoritm::ProfilePoint *ProfRPV,const ClimatData* cl,
                                               OutData::VLayer &V0200_1v, Calc_error::Eps &Eps1)
{
    try{
    int ind_i=0;
    int ind_i_pl_1=0;
    double hm= ProfRPV[i_m].height;
  //  Calc_error::Eps Eps1;
    //Если найдены соседние уровни, между которыми находится уровень 200м
    if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level200,ind_i,ind_i_pl_1)&&
            FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,ind_i,ind_i_pl_1)){
        FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,ind_i, ind_i_pl_1, Constants::level200, V0200_1v.Vd);
        GetParams_0_200_1v(ProfRPV,i_m,V0hmsr,V0200_1v.Vd,V0200_1v.Vsr);//110
        ClimatData::Sigma Si;
        //!si Climat::Get_Si(Constants::level200,Climat::Zone,Si);
        Si=cl->sigma(Constants::level200);
        Eps1=Calc_error::Get_Eps(Si,0,Constants::level200,ProfRPV);
    }
    //Если интерполяция невозможна
    else{//существует ли уровень измерения h(m+1)
       bool b1=(i_m<(InData::k-1));
       bool b2=false;
       if(b1) b2=(ProfRPV[i_m+1].curr==1);
       if (b1 && b2){
           OutData::V Vhm;
           FunctionsCalc::Get_VZ_VM( ProfRPV[i_m].speed, ProfRPV[i_m].azimut,Vhm);
           V0200_1v.Vsr=paramsvdvsr_0_200m__3_3p.GetV0200_f100(ProfRPV,i_m,Vhm,V0hmsr);
           double hmpl1= ProfRPV[i_m+1].height;
           double Yl=Constants::level200;
           Eps1=Calc_error::Get_Eps_f102( cl, Yl, hm, hmpl1);//ф.102
       }
       else{//если уровень измерения h(m+1) отсутствует
           if (hm>=180)
               V0200_1v.Vsr=V0hmsr;
           else V0200_1v.Vsr=V100d;//112
           Eps1=GetEps0200(cl);
       }
    }
    return true;
   }
    catch(...){
        return false;
    }
}

//Первый вариант оценки параметров среднего ветра в слое 0-200м (ф. 110)
//i_m - индекс измерения на высоте hm;
//VZ_0_hm, VM_0_hm - скорость в слое 0-hm;
//VZ_200, VM_200 - скорость на уровне 200м;
//VZ_0_200_1v, VM_0_200_1v - скорость в слое 0-200 м (вариант 1)
bool ParamsVdVsr_0_200m__3_4p::GetParams_0_200_1v(PlowAlgoritm::ProfilePoint* ProfRPV, int i_m, OutData::V V0hmsr,OutData::V V200d,OutData::V &V0200sr_1v){
    try{
        double hm= ProfRPV[i_m].height;
        double V_hm= ProfRPV[i_m].speed;
        double TETA_hm= ProfRPV[i_m].azimut;
        OutData::V Vhm;
        FunctionsCalc::Get_VZ_VM(V_hm,TETA_hm,Vhm);
        V0200sr_1v.VZ=(V0hmsr.VZ*hm+0.5*(Vhm.VZ+V200d.VZ)*(Constants::level200-hm))/Constants::level200;
        V0200sr_1v.VM=(V0hmsr.VM*hm+0.5*(Vhm.VM+V200d.VM)*(Constants::level200-hm))/Constants::level200;

        return true;
    }
    catch(...){
        return false;
    }
}

Calc_error::Eps ParamsVdVsr_0_200m__3_4p::GetEps0200(const ClimatData* cl){
    Calc_error::Eps Eps0200;
    ClimatData::Sigma Si;
    //!si Climat::Get_Si(Constants::level200,Climat::Zone,Si);
    Si=cl->sigma(Constants::level200);
    Eps0200.EpsZ=Si.SiVZ;
    Eps0200.EpsM=Si.SiVM;
    return Eps0200;
}




