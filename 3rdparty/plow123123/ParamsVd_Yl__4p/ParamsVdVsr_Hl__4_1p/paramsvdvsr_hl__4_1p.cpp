#include "paramsvdvsr_hl__4_1p.h"
#include "ParamsVd_Yl__4p/paramsvd_yl__4p.h"
#include "OutData/outdata.h"

#include "InData/indataclimat.h"

#include "InData/Constants.h"
#include "ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h"
#include "ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h"
#include "FunctionsCalc/functionscalc_layer.h"
#include "ParamsVdVsr_0_200m__3_4p/paramsvdvsr_0_200m__3_4p.h"

#include<iostream>

int ParamsVdVsr_Hl__4_1p::IPRL=0;

ParamsVdVsr_0_200m__3_3p paramsvdvsr_0_200m__3_3p1;
ParamsVdVsr_0_200m__3_4p paramsVdVsr_0_200m__3_4p1;
ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p6;

ParamsVdVsr_Hl__4_1p::ParamsVdVsr_Hl__4_1p()
{

}

bool ParamsVdVsr_Hl__4_1p::Do_4_1p(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int l,double &Hl,OutData::VLayer &V0Hl){
    try{
        //Для проверки
        std::cout<<"ParamsVd_Yl__4p::Hlmas[l-2] "<<ParamsVd_Yl__4p::Hlmas[l-2]<<" l="<< Constants::StandartLayer[l-1]<<
                   std::endl;
        //-----------
        double Yl=Constants::StandartLayer[l-1];
        int im=-1;
        FunctionsCalc::Poisk_hm(ProfRPV,Yl,im);
        Hl=0;
        if (im>=0) Hl= ProfRPV[im].height;
        //Если Yl совпадает с каким-либо уровнем измерения локатора
        int _im=-1;
        _im=GetHl(ProfRPV,Yl);
        if(_im>=0) Hl=ProfRPV[_im].height;
        //Если не сущ. уровня измерения локатора Hl, ближайшего
        //к верхней границе станд. слоя Yl
        if (((_im<0)&&(im<0))||(Hl<=Constants::level200)){
            Hl=Constants::level200;
            V0Hl.Vsr=ParamsVdVsr_0_200m__3p::V0200_3p.Vsr;
            V0Hl.Vd=ParamsVdVsr_0_200m__3p::V0200_3p.Vd;
            IPRL=0;
        }
        if (Hl>Constants::level200){
            if (l>2){
                if (Hl==ParamsVd_Yl__4p::Hlmas[l-2]){
                    V0Hl=ParamsVd_Yl__4p::V0Hl[l-2];
                    ParamsVd_Yl__4p::Hlmas[l-1]=Hl;
                }
                if (Hl>ParamsVd_Yl__4p::Hlmas[l-2]){
                  if ((ParamsVd_Yl__4p::V0Hl[l-2].Vd.VZ!=Constants::UNDEF_V)&& (ParamsVd_Yl__4p::V0Hl[l-2].Vd.VM!=Constants::UNDEF_V))
                   V0Hl=GetV0Hlsr(ProfRPV,Hl,l);//116
                   else
                      V0Hl=GetV0Hlsr_l2(ProfRPV,cl,Hl);//!!!!!!!!!!!!!!
                }
               }
            if ((l==2)||((ParamsVd_Yl__4p::V0Hl[l-2].Vd.VZ==Constants::UNDEF_V)&&
                         (ParamsVd_Yl__4p::V0Hl[l-2].Vd.VM==Constants::UNDEF_V))){
                   V0Hl=GetV0Hlsr_l2(ProfRPV,cl,Hl);
            }
        }
        if ((V0Hl.Vd.VZ==Constants::UNDEF_V) && ((V0Hl.Vd.VM==Constants::UNDEF_V)))
          IPRL=0;
        ParamsVd_Yl__4p::Hlmas[l-1]=Hl;
        ParamsVd_Yl__4p::V0Hl[l-1]=V0Hl;

        return true;
    }
    catch(...){
        return false;
    }
}

int ParamsVdVsr_Hl__4_1p::GetHl(PlowAlgoritm::ProfilePoint* ProfRPV,double Yl){
    int im=-1;
    for(int i=0;i<InData::k;i++){
        if(Yl==ProfRPV[i].height)
            if(im!=0)
              im=i-1;
    }
    return im;
}

//116
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, int l){
  OutData::VLayer V0Hlsr;
  double h00=0;
  if (InData::bottomLayer)
      h00=InData::h0;

  double Hlm1=ParamsVd_Yl__4p::Hlmas[l-2];
  int iHl,iHlm1;
 /* FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
  FunctionsCalc::Poisk_hm(ProfRPV,Hlm1,iHlm1);*/
  iHl=FunctionsCalc::Get_indh(ProfRPV, Hl);
  iHlm1=FunctionsCalc::Get_indh(ProfRPV, Hlm1);
  int M= iHl-iHlm1-1;
  int num=M+2;

  L Lmas[num];
  double sumZ=0, sumM=0;
  if((iHlm1<0)){
      iHlm1=0;
      int i200pl;//индекс элемента профиля ветра с высотой >200м
      FunctionsCalc::Poisk_hm_ontop(ProfRPV, Constants::level200, i200pl);
      M = iHl-iHlm1-i200pl/*1*/;
      num=M+2;
      Lmas[iHlm1].Vi=ParamsVdVsr_0_200m__3p::V0200_3p.Vd;
      Lmas[iHlm1].hi=Constants::level200;

      for (int i=1;i<num;i++){
          FunctionsCalc::Get_VZ_VM(ProfRPV[i200pl+i-1/*i-1*/].speed, ProfRPV[i200pl+i-1/*i-1*/].azimut,Lmas[i].Vi);
          Lmas[i].hi= ProfRPV[/*i-1*/i200pl+i-1].height;
      }
      for (int n=0;n<(num-1);n++){
          sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
          sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
      }
  }
  else{
   for (int i=iHlm1;i<=iHl;i++){
       FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i-iHlm1].Vi);
       Lmas[i-iHlm1].hi= ProfRPV[i].height;

   }
     for (int n=0;n<=M;n++){
       sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
       sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
   }
  }
  V0Hlsr.Vsr.VZ=(ParamsVd_Yl__4p::V0Hl[l-2].Vsr.VZ*(Hlm1-h00)+0.5*sumZ)/(Hl-h00);
  V0Hlsr.Vsr.VM=(ParamsVd_Yl__4p::V0Hl[l-2].Vsr.VM*(Hlm1-h00)+0.5*sumM)/(Hl-h00);
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

////116ан
//OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_an(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl){
//  OutData::VLayer V0Hlsr;
//  double h00=0;
//  if (InData::bottomLayer)
//      h00=InData::h0;
//  int iHl;
//  FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
//  int M=iHl+1;
//  int num=M+1;
//  L Lmas[num];
//  Lmas[0].Vi=ParamsVdVsr_0_200m__3p::V0200_3p.Vd;
//  Lmas[0].hi=Constants::level200;
//  for (int i=0;i<=iHl;i++){
//      FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i+1].Vi);
//      Lmas[i+1].hi= ProfRPV[i].height;
//  }
//  double sumZ=0, sumM=0;
//  for (int n=0;n<M;n++){
//      sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
//      sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
//  }
//  V0Hlsr.Vsr.VZ=(ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VZ*(Constants::level200-h00)+0.5*sumZ)/(Hl-h00);
//  V0Hlsr.Vsr.VM=(ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VM*(Constants::level200-h00)+0.5*sumM)/(Hl-h00);
//  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
//  return V0Hlsr;
//}

//116ан
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_an(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl){
  OutData::VLayer V0Hlsr;
  double h00=0;
  if (InData::bottomLayer)
      h00=InData::h0;
  int iHl,i200ontop;
  FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
  FunctionsCalc::Poisk_hm_ontop(ProfRPV,Constants::level200,i200ontop);
  int M,num;
  if(ProfRPV[i200ontop].height>=Hl){
      i200ontop=iHl;
      M=0; num=2;
  }
  else{
      M=iHl-i200ontop;
      num=M+2;
  }
  L Lmas[num];
  Lmas[0].Vi=ParamsVdVsr_0_200m__3p::V0200_3p.Vd;
  Lmas[0].hi=Constants::level200;
  for (int i=i200ontop;i<=iHl;i++){
      FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i-i200ontop+1].Vi);
      Lmas[i-i200ontop+1].hi= ProfRPV[i].height;
  }
  double sumZ=0, sumM=0;
  for (int n=0;n<=M;n++){
      sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
      sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
  }
  V0Hlsr.Vsr.VZ=(ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VZ*(Constants::level200-h00)+0.5*sumZ)/(Hl-h00);
  V0Hlsr.Vsr.VM=(ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VM*(Constants::level200-h00)+0.5*sumM)/(Hl-h00);
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

//116ан2
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_an2(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, OutData::V V0h1){
  OutData::VLayer V0Hlsr;
  double h00=0;
  if (InData::bottomLayer)
      h00=InData::h0;

  double h1=ProfRPV[0].height;
  int iHl;
  FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
  int M= iHl+1;
  int num=M+1;

  L Lmas[num];
  Lmas[0].Vi=ParamsVdVsr_0_200m__3p::V0200_3p.Vd;
  Lmas[0].hi=Constants::level200;
  for (int i=0;i<=iHl;i++){
      FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i+1].Vi);
      Lmas[i+1].hi= ProfRPV[i].height;
  }

  double sumZ=0, sumM=0;
  for (int n=0;n<M;n++){
      sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
      sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
  }
  V0Hlsr.Vsr.VZ=(V0h1.VZ*(h1-h00)+0.5*sumZ)/(Hl-h00);
  V0Hlsr.Vsr.VM=(V0h1.VM*(h1-h00)+0.5*sumM)/(Hl-h00);
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

//116ан3
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_an3(PlowAlgoritm::ProfilePoint* ProfRPV, /*const ClimatData* cl,*/ double Hl, OutData::V V0h1){
  OutData::VLayer V0Hlsr;
  double h00=0;
  if (InData::bottomLayer)
      h00=InData::h0;
  double h1=ProfRPV[0].height;
  OutData::V Vh1;
  FunctionsCalc::Get_VZ_VM(ProfRPV[0].speed,ProfRPV[0].azimut,Vh1);
  int iHl;
  FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
  int M= iHl+1;
  int num=M+1;

  L Lmas[num];
  Lmas[0].Vi=Vh1;
  Lmas[0].hi=h1;
  for (int i=0;i<=iHl;i++){
      FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i+1].Vi);
      Lmas[i+1].hi= ProfRPV[i].height;
  }

  double sumZ=0, sumM=0;
  for (int n=0;n<M;n++){
      sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
      sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
  }
  V0Hlsr.Vsr.VZ=(V0h1.VZ*(h1-h00)+0.5*sumZ)/(Hl-h00);
  V0Hlsr.Vsr.VM=(V0h1.VM*(h1-h00)+0.5*sumM)/(Hl-h00);
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

//Если l=2 или не определена скорость действ. ветра на предыдущем уровне
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_l2(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, double Hl){
    OutData::VLayer V0Hlsr;
    int iHl,M;
    OutData::V VHl;
    FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
    FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  //Если есть приземные данные
  if (InData::bottomLayer){
      paramsvdvsr_0_200m__3_3p1.GetV0layer(ProfRPV,Hl,InData::V0ZM,/*VHl*/V0Hlsr.Vd,M,V0Hlsr.Vsr);//117,98
  }
  else{//если приземные данные отсутствуют
      OutData::VLayer V0200=ParamsVdVsr_0_200m__3p::V0200_3p;
      //Если имеются данные о действ. ветре на 200м
      if ((V0200.Vd.VZ!=Constants::UNDEF_V)&&(V0200.Vd.VM!=Constants::UNDEF_V)){
         V0Hlsr=GetV0Hlsr_an(ProfRPV/*,cl*/,Hl);
      }
      else{
          if (iHl==0){//если Hl явл. 1-ым уровнем измерения локатора
              V0Hlsr.Vsr=GetV0Hl(ProfRPV,cl,iHl);//по методике п.3.4
          }
          else{//если Hl НЕ явл. 1-ым уровнем измерения локатора
             OutData::V V0h1=GetV0Hl(ProfRPV,cl,0);//??????????????
             V0Hlsr=GetV0Hlsr_an3(ProfRPV,/*cl,*/ Hl, V0h1);
          }
      }
  }
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

//?????Проверить!!!! Сверить потом с обновленной версией алгоритма
//Для низлежащих уровней из п.4.4.4
OutData::VLayer ParamsVdVsr_Hl__4_1p::GetV0Hlsr_l2_niz(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, double Hl){
    OutData::VLayer V0Hlsr;
    int iHl,M;
    OutData::V VHl;
    FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
    FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
    bool b1=(Hl>Constants::level200);
    bool b2=((ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VZ!=Constants::UNDEF_V)&&
             (ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VM!=Constants::UNDEF_V));
    if(b1 && b2)
        V0Hlsr=GetV0Hlsr_an(ProfRPV,Hl);//117а
    else{
        if (InData::bottomLayer)
            paramsvdvsr_0_200m__3_3p1.GetV0layer(ProfRPV,Hl,InData::V0ZM,/*VHl*/V0Hlsr.Vd,M,V0Hlsr.Vsr);//117,98
        else{
            if (iHl==0){//если Hl явл. 1-ым уровнем измерения локатора
                V0Hlsr.Vsr=GetV0Hl(ProfRPV,cl,iHl);//по методике п.3.4
            }
            else{//если Hl НЕ явл. 1-ым уровнем измерения локатора
               OutData::V V0h1=GetV0Hl(ProfRPV,cl,0);//??????????????
               //V0Hlsr=GetV0Hlsr_an2(ProfRPV,cl, Hl, V0h1);
               V0Hlsr=GetV0Hlsr_an3(ProfRPV,/*cl,*/ Hl, V0h1);
            }
        }
    }
  return V0Hlsr;
}

OutData::V ParamsVdVsr_Hl__4_1p::GetV0Hl(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int i_m){
    OutData::V V0hm;
    Calc_error::Eps Eps0hm;
    if(InData::k>=3)
        Method0Hl(ProfRPV,cl,(i_m+1), V0hm, Eps0hm);
    if(InData::k==2){
        OutData::V *VZM=new OutData::V [InData::k];
        paramsVdVsr_0_200m__3_1p6.GetMasVZM(ProfRPV,VZM);
        FunctionsCalc::Gr gr[InData::k];
        for(int i=0;i<InData::k;i++)
         if(i!=i_m)
             gr[i]=FunctionsCalc::GetGr(VZM[i],VZM[i_m],ProfRPV[i].height,ProfRPV[i_m].height);
        V0hm=GetV0hm_1v(ProfRPV,cl, i_m,gr,Eps0hm);
    }
    return V0hm;
}

//Методика для расчета среднего ветра в слое 0-Hl
bool ParamsVdVsr_Hl__4_1p::Method0Hl(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int n1, OutData::V &V0200_1,Calc_error::Eps &Eps1){
   OutData::V *VZM=new OutData::V [InData::k];
   paramsVdVsr_0_200m__3_1p6.GetMasVZM(ProfRPV,VZM);
   int nk=0;
   std::list<int>::iterator it;
   FunctionsCalc::Gr gr[InData::k];
   list<int> MK=paramsVdVsr_0_200m__3_1p6.FormMK(n1, ProfRPV, VZM,gr,nk);
   if(nk==0){
       V0200_1=GetV0hm_1v(ProfRPV,cl, n1-1,gr,Eps1);
       delete [] VZM;
       return true;
   }
   it=MK.begin();
   //Массив параметров среднего ветра в слое 0-200
   OutData::V V0200[nk];
  // Calc_error::Eps Eps[nk];
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
           V0hn1=paramsVdVsr_0_200m__3_1p6.GetV0hn1_1v(Vi, Vj, Vn1, hi, hj, hn1);
       if((hi>hn1) && (hj>hn1))
           V0hn1=paramsVdVsr_0_200m__3_1p6.GetV0hn1_2v(Vi, Vj, Vn1, hi, hj, hn1);
       //if((hi>hn1) && (hj<hn1) || (hi<hn1) && (hj>hn1))
       if(((hi>hn1) && (hj<hn1)) || ((hi<hn1) && (hj>hn1)))
           V0hn1=paramsVdVsr_0_200m__3_1p6.GetV0hn1_3v(Vi, Vj, Vn1, hi, hj);

       V0200[c].VZ=V0hn1.VZ;
       V0200[c].VM=V0hn1.VM;
   }

   double V0200Z=0, V0200M=0;
   for(int c=0;c<nk;c++){
    V0200Z+=V0200[c].VZ;
    V0200M+=V0200[c].VM;
   }
   //ф.45а
   V0200_1.VZ=V0200Z/nk;
   V0200_1.VM=V0200M/nk;
   delete [] VZM;
   return true;
}

OutData::V ParamsVdVsr_Hl__4_1p::GetV0hm_1v(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int im, FunctionsCalc::Gr* gr, Calc_error::Eps &Epsm){
    OutData::V V0hm;
    double masgr[InData::k];
    double sum=0;
    for(int i=0;i<InData::k;i++){
        sum=pow(gr[i].Z,2)+ pow(gr[i].M,2);
        masgr[i]=pow(sum,0.5);
    }
    int j=0;
    MathFunc::Poisk_mind(masgr,InData::k,j);
    double hj=ProfRPV[j].height;
    if(hj>Constants::level200)
       V0hm=paramsVdVsr_0_200m__3_4p1.GetV0hm_f107(ProfRPV, cl,im, j, Epsm);//ф.107
    return V0hm;
}











