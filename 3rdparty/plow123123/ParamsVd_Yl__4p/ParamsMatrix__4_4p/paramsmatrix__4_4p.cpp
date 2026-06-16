#include "paramsmatrix__4_4p.h"
#include "InData/InData.h"
#include "math.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h"
#include "ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h"
#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"
#include "FunctionsCalc/functionscalc_layer.h"
#include "ParamsVdVsr_0_200m__3_4p/paramsvdvsr_0_200m__3_4p.h"
#include "InData/indataclimat.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"
#include "ParamsVd_Yl__4p/paramsvd_yl__4p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"
#include "BottomEval__2p/bottomeval__2p.h"
#include "FunctionsCalc/calc_bottomlayer.h"

#include "stdlib.h"
#include <stdio.h>
#include <iostream>

using namespace std;
#include <fstream>

ParamsVdVsr_0_200m__3_3p  paramsVdVsr_0_200m__3_3p1;
ParamsVdVsr_Hl__4_1p paramsVdVsr_Hl__4_1p1;
ParamsVdVsr_0_200m__3_4p  paramsVdVsr_0_200m__3_4p2;
ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p5;
ParamsdelV__4_3p paramsdelV__4_3p1;
BottomEval__2p bottomEval__2p4;

const int ParamsMatrix__4_4p::numMod=7;

ParamsMatrix__4_4p::ParamsMatrix__4_4p()
{

}

void ParamsMatrix__4_4p::GetFormMatrix(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int l,
                                       double Yl,double Hl,OutData::VLayer V0Hl,OutData::V** VCast,
                                       int &NN,int iHl, int ihn,M **MZ, M **MM,int &kmod/*, double HH, OutData::VLayer &V0HH*/){
   int Nl=GetNl_4_4_1p(ProfRPV,Yl);
   int j1, j2;
   double HH;
   OutData::VLayer V0HH;
   if (Nl>=4){
       bool m4,m3;
       int i1,i2,i3;
       Get4lModel_4_4_2p(ProfRPV,Yl,Nl,m4,m3,i1,i2,i3);
       if (m4){
           Do4lModel_4_4_4p(ProfRPV,cl,i1,i2,i3,l,Yl,Hl,V0Hl,NN,iHl,ihn,VCast,MZ,MM,HH,V0HH);
           V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
           Hl=ParamsVd_Yl__4p::Hlmas[l-1];

       }
       else if (m3){
           if (Check3lModel_4_4_3p(ProfRPV,Nl,Yl,j1,j2)){
               Do4lModel_4_4_7p(ProfRPV,cl,j1, j2,l,Yl,Hl,V0Hl,NN,iHl,ihn,VCast,MZ,MM,HH,V0HH);//4.4.7
               V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
               Hl=ParamsVd_Yl__4p::Hlmas[l-1];
           }
           else{
               GetDel_4_4_5p(ProfRPV,cl,l,Yl,Hl,V0Hl.Vsr,VCast,NN, MZ, MM);//4.4.5
           }
       }
              else{
            GetDel_4_4_5p(ProfRPV,cl,l,Yl,Hl,V0Hl.Vsr,VCast,NN, MZ, MM);//4.4.5;//4.4.5
       }
   }
   else{
       if (Nl>=2){
           if (Check3lModel_4_4_3p(ProfRPV,Nl,Yl,j1,j2)){
               Do4lModel_4_4_7p(ProfRPV,cl,j1, j2,l,Yl,Hl,V0Hl,NN,iHl,ihn,VCast,MZ,MM,HH,V0HH);//4.4.7
               V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
               Hl=ParamsVd_Yl__4p::Hlmas[l-1];
           }
           else{
                GetDel_4_4_5p(ProfRPV,cl,l,Yl,Hl,V0Hl.Vsr,VCast,NN, MZ, MM);//4.4.5
           }
       }
       else{
            GetDel_4_4_5p(ProfRPV,cl,l,Yl,Hl,V0Hl.Vsr,VCast,NN, MZ, MM);//4.4.5
       }
   }
   FormMatrix_4_4_6p(MZ,MM,numMod,NN);//4.4.6

   kmod=0;
   for (int j=0;j<numMod;j++){
       //if (MZ[j][0].Eps!=ParamsMatrix__4_4p::epsNoMod)
       if (MZ[j][0].delV!=Constants::UNDEF_V)
           kmod++;
   }
   //4.5
}

//Получает количество уровней измерения локатора в пределах слоя 0-Yl
int ParamsMatrix__4_4p::GetNl_4_4_1p(PlowAlgoritm::ProfilePoint* ProfRPV, double Yl){
    int Nl=0;
    for (int i=0;i<InData::k;i++){
        if ( ProfRPV[i].height<=Yl)
            Nl++;
        else break;
    }
    return Nl;
}

//Расценивает возможность реализации 4-х слойной и 3-х слойной моделей
//m4=true - возможно реализовать 4-х слойную модель;
//m3=true - возможно реализовать 3-х слойную модель.
void ParamsMatrix__4_4p::Get4lModel_4_4_2p(PlowAlgoritm::ProfilePoint* ProfRPV, double Yl,int Nl, bool &m4, bool &m3,int &i1, int &i2, int &i3){
   /*if (Yl==618)
       int k=0;*/
    m3=false; m4=false;
   double y1,y2,y3;//133
   y1=Yl/4;
   y2=2*y1;
   y3=3*y1;
   double hmas[Nl];
   for (int i=0;i<Nl;i++)
       hmas[i]= ProfRPV[i].height;
   double hsh1;
   int k=0;
   int imin=0;
   while(k<Nl){
           hsh1=hsh(Nl, y1, hmas,imin);
           if (hsh1>=150)
               break;
           if (imin<0)
               break;
           hmas[imin]=-1;
           k++;
   }
   i1=imin;
   if(hsh1<150){
       m4=false;
       m3=false;
       return;
   }
   double hsh2=-1;
   k=0;
   for (int i=0;i<Nl;i++)
       hmas[i]= ProfRPV[i].height;
   while(k<Nl){
           hsh2=hsh(Nl, y2, hmas,imin);
           if (hsh2>hsh1)
               break;
           if (imin<0)
               break;
           hmas[imin]=-1;
           k++;
   }
   i2=imin;
   if (hsh2<=hsh1){
       m4=false;
       m3=true;
       return;
   }
   double hsh3=-1;
   for (int i=0;i<Nl;i++)
       hmas[i]= ProfRPV[i].height;
   k=0;
   while(k<Nl){
           hsh3=hsh(Nl, y3, hmas,imin);
           if (hsh3>hsh2)
               break;
           if (imin<0)
               break;
           hmas[imin]=-1;
           k++;
   }
   i3=imin;
   if (hsh3<=hsh2){
       m4=false;
       m3=true;
       return;
   }
   m3=true;
   double SG=GetSG(y1,Yl,hsh1,hsh2,hsh3);
   if ((SG/y1)>0.33) m4=false;
     else m4=true;
}

double ParamsMatrix__4_4p::hsh(int Nl, double y, double hmas[],int &imin){
    double hsh=-1;
    double min=-1;
    imin=-1;
    for (int i=0;i<Nl;i++)
        if (hmas[i]>0){
            hsh=hmas[i];
            imin=i;
            min=abs(hmas[i]-y);
            break;
        }
    if (hsh>0)
     for (int i=(imin+1);i<Nl;i++){
        if (hmas[i]>0)
         if (abs(hmas[i]-y)<min){
            hsh=hmas[i];
            min=abs(hmas[i]-y);
            imin=i;
        }
     }
    return hsh;
}

double ParamsMatrix__4_4p::GetSG(double y1, double Yl, double hsh1,double hsh2,double hsh3){
    double SG;
    double h00=0;
    if (InData::bottomLayer)
        h00=InData::h0;
    SG=(pow((hsh1-h00-y1),2)+pow((hsh2-hsh1-y1),2)+pow((hsh3-hsh2-y1),2)+pow((Yl-hsh3-y1),2))/3;
    SG=pow(SG,0.5);
    return SG;
}

//Получает максимальное кол-во шагов, на которых могут быть отработаны все возможные модели приращений,+1 - Yl
int ParamsMatrix__4_4p::GetNN(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, int &iHl, int &ihn){
    int NN;
    iHl=-1;
    ihn=-1;
    int nl=Getnl(ProfRPV,Hl,iHl,ihn);
    NN=nl+1;
    return NN;
}

void ParamsMatrix__4_4p::Do4lModel_4_4_4p(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int i1, int i2, int i3,
                                          int l, double Yl,double Hl,OutData::VLayer V0Hl,int &NN, int iHl, int ihn,
                                         OutData::V **VCast, M **MZ, M **MM, double &HH,OutData::VLayer &V0HH){
   OutData::V delV0Yl0hsh3[numMod];
   Calc_error::Eps eps[numMod];
   for (int i=0;i<numMod;i++){
       eps[i].EpsZ=epsNoMod;
       eps[i].EpsM=epsNoMod;
   }
   double delhy=-1;
   OutData::VLayer V0hsh3;
   GetVarMod(ProfRPV,cl,i1, i2, i3,l, Yl,Hl, V0Hl,-1,VCast,delhy,V0hsh3,delV0Yl0hsh3,eps);
   //-----------------------------------------------------------------
  /* int iHl=-1;
   int ihn=-1;
   int nl=Getnl(Hl,iHl,ihn);
   NN=nl+1;//максимальное кол-во шагов, на которых могут быть отработаны все возможные модели приращений,+1 - Yl*/
   int nl=NN-1;

   std::cout<<"Prirazheniya:"<<std::endl;
   for (int j=0;j<numMod;j++){
       MZ[j][0].delV=delV0Yl0hsh3[j].VZ;
       MZ[j][0].Eps=eps[j].EpsZ;
       MM[j][0].delV=delV0Yl0hsh3[j].VM;
       MM[j][0].Eps=eps[j].EpsM;
       //Для проверки
       std::cout<<j+1<<"\t"<<MZ[j][0].delV<<"\t\t"<<MZ[j][0].Eps<<"\t"<<MM[j][0].delV<<"\t\t"<<MM[j][0].Eps<<std::endl;
       //--------------
   }


   int Nl;
   bool m4,m3;
   int ii1,ii2,ii3;
   int N=1;//кол-во шагов, на которых могут быть отработаны все возможные модели приращений
   double hi=0;
   double delhyOut=-1;
   //double h2sh3=0;
   OutData::VLayer V0hi,V0h2sh3;
   OutData::V delV0hi0h2sh3;
   double epsji;
   bool b1=((nl>0)&&(Hl>=1000));
   if(!b1) NN=1;//!!!posle
   //if (nl>0)//если есть хоть 1 уровень ниже Yl
   if(b1)
    for (int i=iHl;i>ihn;i--){
     hi= ProfRPV[i].height;
     Nl=GetNl_4_4_1p(ProfRPV,hi);
     if (Nl>=4){
       Get4lModel_4_4_2p(ProfRPV,hi,Nl,m4,m3,ii1,ii2,ii3);
       if (m4){
            Hl= ProfRPV[i-1].height;
            V0Hl=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2_niz(ProfRPV,cl,Hl);
            GetVarMod(ProfRPV,cl,ii1,ii2,ii3,l,hi,Hl,V0Hl,delhy,VCast,delhyOut,V0h2sh3,delV0Yl0hsh3,eps);
            N++;
            FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,V0hi.Vd);
            V0hi.Vsr=GetV0hsh1(ProfRPV,cl,hi,V0hi.Vd);

            delV0hi0h2sh3=GetdelV0hi0h2sh3(V0hi.Vsr,V0h2sh3.Vsr);
            for (int j=0;j<7;j++){
            if (delV0Yl0hsh3[j].VZ!=Constants::UNDEF_V){
             MZ[j][N-1].delV=delV0Yl0hsh3[j].VZ;
             epsji=fabs(delV0Yl0hsh3[j].VZ-delV0hi0h2sh3.VZ);//153
             MZ[j][N-1].Eps=epsji;
            }
           if (delV0Yl0hsh3[j].VM!=Constants::UNDEF_V){
            MM[j][N-1].delV=delV0Yl0hsh3[j].VM;
            epsji=fabs(delV0Yl0hsh3[j].VM-delV0hi0h2sh3.VM);
            MM[j][N-1].Eps=epsji;
           }
          }
       }

    }

   }
  // NN=NN-(ihn+1);
   NN=N;
   //---------------
   //!!!Для проверки
   ofstream ffout;
   ffout.open("M_delV.txt");

   ffout<<"MZ.delV:"<<endl;

       for (int i=0;i<NN;i++){
           for (int j=0;j<numMod;j++)
           ffout<<MZ[j][i].delV<<"\t";
       ffout<<endl;
   }
   ffout<<"MM.delV:"<<endl;
       for (int i=0;i<NN;i++){
           for (int j=0;j<numMod;j++)
           ffout<<MM[j][i].delV<<"\t";
       ffout<<endl;
   }
       ffout<<"N="<<N<<endl;
   ffout.close();

   ofstream efout;
   efout.open("M_eps.txt");

   efout<<"MZ.eps:"<<endl;

       for (int i=0;i<NN;i++){
           for (int j=0;j<numMod;j++)
           efout<<MZ[j][i].Eps<<"\t";
       efout<<endl;
   }
   efout<<"MM.eps:"<<endl;
       for (int i=0;i<NN;i++){
           for (int j=0;j<numMod;j++)
           efout<<MM[j][i].Eps<<"\t";
       efout<<endl;
   }
       efout<<"N="<<N<<endl;
   efout.close();
   //-----------------
   HH=Hl;
   V0HH=V0Hl;
   ParamsdelV__4_3p::IPRHH=1;
}

//152
OutData::V ParamsMatrix__4_4p::GetdelV0hi0h2sh3(OutData::V V0hisr, OutData::V V0h2sh3){
   OutData::V delV0hi0h2sh3;
   delV0hi0h2sh3.VZ=V0hisr.VZ-V0h2sh3.VZ;
   delV0hi0h2sh3.VM=V0hisr.VM-V0h2sh3.VM;
   return delV0hi0h2sh3;
}

void ParamsMatrix__4_4p::GetVarMod(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int i1, int i2, int i3,
                                   int l,double Yl,double Hl,OutData::VLayer V0Hl,double delhyIn,OutData::V** VCast,
                                   double &delhyOut,OutData::VLayer &V0hsh3 , OutData::V *delV0Yl0hsh3,Calc_error::Eps *eps){

    delhyOut=-1;
    double hsh1= ProfRPV[i1].height;
    double hsh2= ProfRPV[i2].height;
    double hsh3= ProfRPV[i3].height;
    OutData::VLayer V0hsh1,V0hsh2;
    FunctionsCalc::Get_VZ_VM( ProfRPV[i1].speed, ProfRPV[i1].azimut,V0hsh1.Vd);
    FunctionsCalc::Get_VZ_VM( ProfRPV[i2].speed, ProfRPV[i2].azimut,V0hsh2.Vd);
    FunctionsCalc::Get_VZ_VM( ProfRPV[i3].speed, ProfRPV[i3].azimut,V0hsh3.Vd);
    V0hsh1.Vsr=GetV0hsh1(ProfRPV,cl,hsh1,V0hsh1.Vd);
    V0hsh2.Vsr=GetV0Hlsr_f136(ProfRPV,hsh1,hsh2,V0hsh1.Vsr).Vsr;//136
    V0hsh3.Vsr=GetV0Hlsr_f136(ProfRPV,hsh2,hsh3,V0hsh2.Vsr).Vsr;//137
    if (delhyIn<0){//??????????????
        //Переопределение параметров в конце блока 4.4.4 c.155
        ParamsVd_Yl__4p::HH=ParamsVd_Yl__4p::Hlmas[l-1];
        ParamsVd_Yl__4p::V0HH=ParamsVd_Yl__4p::V0Hl[l-1];
        ParamsVd_Yl__4p::Hlmas[l-1]=hsh3;
        ParamsVd_Yl__4p::V0Hl[l-1]=V0hsh3;
    }
    ClimatData::Pr pr0hsh1cl,pr0hsh2cl,pr0hsh3cl;
    OutData::V V0hsh1cl,V0hsh2cl,V0hsh3cl;
    pr0hsh1cl=GetV0hshcl(cl,hsh1);
    pr0hsh2cl=GetV0hshcl(cl,hsh2);
    pr0hsh3cl=GetV0hshcl(cl,hsh3);
    V0hsh1cl.VZ=pr0hsh1cl.VZ;V0hsh1cl.VM=pr0hsh1cl.VM;
    V0hsh2cl.VZ=pr0hsh2cl.VZ;V0hsh2cl.VM=pr0hsh2cl.VM;
    V0hsh3cl.VZ=pr0hsh3cl.VZ;V0hsh3cl.VM=pr0hsh3cl.VM;

    OutData::V delVrcl21,delVrcl32;
    delVrcl21=GetdelVrcl(/*hsh2,hsh1,*/V0hsh2.Vsr,V0hsh1.Vsr,V0hsh2cl,V0hsh1cl);
    delVrcl32=GetdelVrcl(/*hsh3,hsh2,*/V0hsh3.Vsr,V0hsh2.Vsr,V0hsh3cl,V0hsh2cl);

    double h00=0;
    if(InData::bottomLayer)
        h00=InData::h0;
    double delh1, delh2,delh3,delh4;
    delh1=hsh1-h00;
    delh2=hsh2-hsh1;
    delh3=hsh3-hsh2;
    delh4=Yl-hsh3;
    OutData::V A,B,B1,A2,B2;
    GetKoeff(delh1,delh2,delh3,delh4,delVrcl21,delVrcl32,A,B,B1,A2,B2);
    ClimatData::Pr V0Ylcl;
    OutData::V delV0Yl0hsh3cl;


    int Kmod;
    int im;
   // double delhy;
    if (delhyIn<0){//для Yl
      //!pr  Climat::Get_Vcl(Yl,Climat::Zone,V0Ylcl);
        //V0Ylcl=cl->pr(Yl);
        Climat::Get_Vcl_h(cl,Yl,V0Ylcl);
        delV0Yl0hsh3cl.VZ=V0Ylcl.VZ-V0hsh3cl.VZ;
        delV0Yl0hsh3cl.VM=V0Ylcl.VM-V0hsh3cl.VM;
        delV0Yl0hsh3[0]=GetdelV0Yl0hsh3_1v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A,B,eps[0]);
        delV0Yl0hsh3[1]=GetdelV0Yl0hsh3_2v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,delVrcl32,eps[1]);
        delV0Yl0hsh3[2]=GetdelV0Yl0hsh3_3v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A,B1,eps[2]);
        delV0Yl0hsh3[3]=GetdelV0Yl0hsh3_4v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A2,B2,eps[3]);
        delV0Yl0hsh3[4]=GetdelV0Yl0hsh3_5v(cl,Yl,hsh3,delV0Yl0hsh3cl,V0hsh3.Vsr,V0hsh3cl, eps[4]);
        Kmod=5;
     if (FunctionsCalc::Poisk_hm_ontop(ProfRPV,Yl,im)){
        if(ProfRPV[im].curr==1){
          delV0Yl0hsh3[5]=GetdelV0Yl0hsh3_6v(ProfRPV,cl, im,Yl,Hl,h00,V0Hl,V0hsh3.Vsr,eps[5],delhyOut);
          Kmod++;
        }
     }
     if(delhyOut<0)delhyOut=0;
    }
     else{//для низлежащих уровней (относительно Yl)
        V0Ylcl=GetV0hshcl(cl,Yl);
        delV0Yl0hsh3cl.VZ=V0Ylcl.VZ-V0hsh3cl.VZ;
        delV0Yl0hsh3cl.VM=V0Ylcl.VM-V0hsh3cl.VM;
        delV0Yl0hsh3[0]=GetdelV0Yl0hsh3_1v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A,B,eps[0]);
        delV0Yl0hsh3[1]=GetdelV0Yl0hsh3_2v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,delVrcl32,eps[1]);
        delV0Yl0hsh3[2]=GetdelV0Yl0hsh3_3v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A,B1,eps[2]);
        delV0Yl0hsh3[3]=GetdelV0Yl0hsh3_4v(cl,Yl,hsh3,delh1,delh2,delh3,delh4,delV0Yl0hsh3cl,A2,B2,eps[3]);
        delV0Yl0hsh3[4]=GetdelV0Yl0hsh3_5v(cl,Yl,hsh3,delV0Yl0hsh3cl,V0hsh3.Vsr,V0hsh3cl,eps[4]);

//       int imsh=FunctionsCalc::Poisk_imsh(Yl/*hi*/+delhyIn);
//       delV0Yl0hsh3[5]=GetdelV0Yl0hsh3_6v(imsh,Yl,Hl,h00,V0Hl,V0hsh3.Vsr,eps[5],delhyOut);
//       Kmod++;
        Kmod=5;
        if(delhyIn>0){
         int imsh=FunctionsCalc::Poisk_imsh(ProfRPV,Yl/*hi*/+delhyIn);
         delV0Yl0hsh3[5]=GetdelV0Yl0hsh3_6v(ProfRPV,cl,imsh,Yl,Hl,h00,V0Hl,V0hsh3.Vsr,eps[5],delhyOut);
         Kmod++;
        }
     }
    if (InData::prevZond){
        delV0Yl0hsh3[6]=GetdelV0Yl0hsh3_7v(cl,Yl,hsh3,V0hsh3.Vsr,VCast,eps[6]);
        Kmod++;
    }
}

//Получает количество уровней в пределах слоя 0-Yl,
//которые больше 400м
int ParamsMatrix__4_4p::Getnl(PlowAlgoritm::ProfilePoint* ProfRPV, double Hl, int &iHl, int &ihn){
    int nl=0;
    if(Hl<=400) return nl;//!!!posle
    iHl=-1;
    ihn=-1;
    for (int i=0;i<InData::k;i++)
        if( ProfRPV[i].height==Hl){
            iHl=i;
            break;
        }
    nl=1;
    for (int i=(iHl-1);i>=0;i--){
        if ( ProfRPV[i].height>400)
            nl++;
        else{
            ihn=i+1;
            break;
        }
    }
    return nl;
}

ClimatData::Pr ParamsMatrix__4_4p::GetV0hshcl(const ClimatData* cl, double hsh){
    ClimatData::Pr V0hshcl;
    if (hsh<Constants::level200)
        //!pr Climat::Get_Vcl(Constants::level200,Climat::Zone,V0hshcl);
        V0hshcl=cl->pr(Constants::level200);
    else Climat::Get_Vcl_h(cl,hsh,V0hshcl);
    return V0hshcl;
}

//138
OutData::V ParamsMatrix__4_4p::GetdelVrcl(/*double h2, double h1,*/ OutData::V V0h2, OutData::V V0h1,OutData::V V0h2cl, OutData::V V0h1cl){
    OutData::V delVrcl;
    delVrcl.VZ=V0h2.VZ-V0h1.VZ-V0h2cl.VZ+V0h1cl.VZ;
    delVrcl.VM=V0h2.VM-V0h1.VM-V0h2cl.VM+V0h1cl.VM;
    return delVrcl;
}

void ParamsMatrix__4_4p::GetKoeff(double delh1,double delh2,double delh3,double delh4,OutData::V delVrcl21,OutData::V delVrcl32,
                                  OutData::V &A,OutData::V &B, OutData::V &B1,OutData::V &A2,OutData::V &B2){
    A.VZ=delh3*delVrcl21.VZ/delh2;
    A.VM=delh3*delVrcl21.VM/delh2;
    B.VZ=delVrcl32.VZ*(delh1+delh2)/delh3;
    B.VM=delVrcl32.VM*(delh1+delh2)/delh3;
    B1.VZ=(2*delVrcl32.VZ*(delh1+delh2))/delh3;
    B1.VM=(2*delVrcl32.VM*(delh1+delh2))/delh3;
    A2.VZ=delVrcl21.VZ*(delh3+delh4)/delh2;
    A2.VM=delVrcl21.VM*(delh3+delh4)/delh2;
    B2.VZ=delVrcl32.VZ*(delh1+delh2)/delh3;
    B2.VM=delVrcl32.VM*(delh1+delh2)/delh3;
}

//139
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_1v(const ClimatData* cl, double Yl, double hsh3, double delh1, double delh2,double delh3,double delh4,
                                                  OutData::V delV0Yl0hsh3cl,OutData::V A,OutData::V B, Calc_error::Eps &Eps1){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3.VZ=delV0Yl0hsh3cl.VZ+(delh4*(A.VZ+B.VZ))/(delh1+delh2+delh3+delh4);//139
    delV0Yl0hsh3.VM=delV0Yl0hsh3cl.VM+(delh4*(A.VM+B.VM))/(delh1+delh2+delh3+delh4);
    Eps1=GetEps(cl,Yl,hsh3,delV0Yl0hsh3,delV0Yl0hsh3cl);
    return delV0Yl0hsh3;
}

//142
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_2v(const ClimatData* cl,double Yl, double hsh3, double delh1, double delh2,double delh3,double delh4,
                                                  OutData::V delV0Yl0hsh3cl,OutData::V delVrcl32, Calc_error::Eps &Eps2){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3.VZ=delV0Yl0hsh3cl.VZ+(delh4*(delh1+delh2)*delVrcl32.VZ)/((delh1+delh2+delh3+delh4)*delh3);
    delV0Yl0hsh3.VM=delV0Yl0hsh3cl.VM+(delh4*(delh1+delh2)*delVrcl32.VM)/((delh1+delh2+delh3+delh4)*delh3);
    Eps2=GetEps(cl,Yl,hsh3,delV0Yl0hsh3,delV0Yl0hsh3cl);
    return delV0Yl0hsh3;
}

//143
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_3v(const ClimatData* cl,double Yl, double hsh3, double delh1, double delh2,double delh3,double delh4,
                                                  OutData::V delV0Yl0hsh3cl,OutData::V A,OutData::V B1, Calc_error::Eps &Eps3){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3.VZ=delV0Yl0hsh3cl.VZ+(delh4*(A.VZ+B1.VZ))/(2*(delh1+delh2+delh3+delh4));//143
    delV0Yl0hsh3.VM=delV0Yl0hsh3cl.VM+(delh4*(A.VM+B1.VM))/(2*(delh1+delh2+delh3+delh4));
    Eps3=GetEps(cl,Yl,hsh3,delV0Yl0hsh3,delV0Yl0hsh3cl);
    return delV0Yl0hsh3;
}

//144
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_4v(const ClimatData* cl,double Yl, double hsh3, double delh1, double delh2,double delh3,double delh4,
                                                  OutData::V delV0Yl0hsh3cl,OutData::V A2,OutData::V B2, Calc_error::Eps &Eps4){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3.VZ=delV0Yl0hsh3cl.VZ+(delh4*(A2.VZ+B2.VZ))/(delh1+delh2+delh3+delh4);//144
    delV0Yl0hsh3.VM=delV0Yl0hsh3cl.VM+(delh4*(A2.VM+B2.VM))/(delh1+delh2+delh3+delh4);
    Eps4=GetEps(cl,Yl,hsh3,delV0Yl0hsh3,delV0Yl0hsh3cl);
    return delV0Yl0hsh3;
}

//145
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_5v(const ClimatData* cl,double Yl, double hsh3, OutData::V delV0Yl0hsh3cl,
                                                  OutData::V V0hsh3, OutData::V V0hsh3cl,Calc_error::Eps &Eps5){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3.VZ=delV0Yl0hsh3cl.VZ;//145
    delV0Yl0hsh3.VM=delV0Yl0hsh3cl.VM;
    Eps5=GetEps_f146(cl,Yl,hsh3,V0hsh3, V0hsh3cl);
    return delV0Yl0hsh3;
}

//147-149
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_6v(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int im, double Yl, double Hl, double h00,
                             OutData::VLayer V0Hl,OutData::V V0hsh3sr,Calc_error::Eps &Eps6, double &delhy){
    OutData::V delV0Yl0hsh3;
    double hm= ProfRPV[im].height;
    OutData::V Vhm;
    FunctionsCalc::Get_VZ_VM(ProfRPV[im].speed, ProfRPV[im].azimut, Vhm);
    delV0Yl0hsh3.VZ=V0Hl.Vsr.VZ+(Vhm.VZ-V0Hl.Vd.VZ)*pow((Yl-Hl),2)/(2*(Yl-h00)*(hm-Hl))+//ф.147
            (Yl-Hl)*(V0Hl.Vd.VZ-V0Hl.Vsr.VZ)/(Yl-h00)-V0hsh3sr.VZ;
    delV0Yl0hsh3.VM=V0Hl.Vsr.VM+(Vhm.VM-V0Hl.Vd.VM)*pow((Yl-Hl),2)/(2*(Yl-h00)*(hm-Hl))+
            (Yl-Hl)*(V0Hl.Vd.VM-V0Hl.Vsr.VM)/(Yl-h00)-V0hsh3sr.VM;
    Eps6=Calc_error::Get_Eps_f102(cl, Yl, Hl, hm);//ф.102
 //   if (delhy<0)//Проверить!!!
        delhy=hm-Yl;
    return delV0Yl0hsh3;
}

OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh3_7v(const ClimatData* cl,double Yl,double h,OutData::V V0Hlsr,OutData::V **VCast,
                                                  Calc_error::Eps &Eps7){
    OutData::V delV0Yl0hsh3;
    delV0Yl0hsh3=GetdelV_7v(cl,Yl, h, V0Hlsr,VCast,Eps7);
    return delV0Yl0hsh3;
}

//140
Calc_error::Eps ParamsMatrix__4_4p::GetEps(const ClimatData* cl,double Yl, double hsh3,
                                           OutData::V delV0Yl0hsh3, OutData::V delV0Yl0hsh3cl){
    Calc_error::Eps eps;
    FunctionsCalc::K k1;//??????????
    if (delV0Yl0hsh3cl.VZ==0)//141
        delV0Yl0hsh3cl.VZ=0.01;
      k1.Z=(delV0Yl0hsh3.VZ-delV0Yl0hsh3cl.VZ)/delV0Yl0hsh3cl.VZ;
    if (delV0Yl0hsh3cl.VM==0)//141
       delV0Yl0hsh3cl.VM=0.01;
      k1.M=(delV0Yl0hsh3.VM-delV0Yl0hsh3cl.VM)/delV0Yl0hsh3cl.VM;
    FunctionsCalc::K K1;
    K1.Z=pow((pow(k1.Z,2)+1),0.5);
    K1.M=pow((pow(k1.M,2)+1),0.5);
    ClimatData::Sigma Si0hsh3,Si0Yl;
    Climat::Get_Si_h(cl,hsh3,Si0hsh3);
    //!si Climat::Get_Si(Yl,Climat::Zone,Si0Yl);
    //Si0Yl=cl->sigma(Yl);
    Climat::Get_Si_h(cl,Yl,Si0Yl);
    ClimatData::Correl CorrYlhsh3;
    double Yk;
    int ik;
    FunctionsCalc::Poisk_hm_ontop_mas(Climat::StandartLayer,15,Yl,ik);
    Yk=Climat::StandartLayer[ik];
    if (hsh3<=Constants::level200)
        Climat::Get_Correl_h_low(cl,Constants::level200,Yk,CorrYlhsh3);//????????
    else {
        Climat::Get_Correl_h_low(cl,Yk,hsh3,CorrYlhsh3);//???????????????
    }
    eps.EpsZ=pow(Si0hsh3.SiVZ,2)+pow(Si0Yl.SiVZ,2)-2*Si0hsh3.SiVZ*Si0Yl.SiVZ*CorrYlhsh3.CorrZ;//140
    if (eps.EpsZ!=0)
      eps.EpsZ=K1.Z*pow(eps.EpsZ,0.5);
    eps.EpsM=pow(Si0hsh3.SiVM,2)+pow(Si0Yl.SiVM,2)-2*Si0hsh3.SiVM*Si0Yl.SiVM*CorrYlhsh3.CorrM;
    if (eps.EpsM!=0)
      eps.EpsM=K1.M*pow(eps.EpsM,0.5);
    return eps;
}

//146
Calc_error::Eps ParamsMatrix__4_4p::GetEps_f146(const ClimatData* cl,double Yl, double hsh3,OutData::V V0hsh3, OutData::V V0hsh3cl){
    Calc_error::Eps eps;
    ClimatData::Sigma Si0hsh3,Si0Yl;
    Climat::Get_Si_h(cl, hsh3,Si0hsh3);
    Climat::Get_Si_h(cl,Yl,Si0Yl);
    ClimatData::Correl CorrYlhsh3;
    double Yk;
    int ik;
    FunctionsCalc::Poisk_hm_ontop_mas(Climat::StandartLayer,15,Yl,ik);
    Yk=Climat::StandartLayer[ik];
    if (hsh3<=Constants::level200)
        Climat::Get_Correl_h_low(cl,Constants::level200,Yk,CorrYlhsh3);//????????
    else {
        Climat::Get_Correl_h_low(cl,Yk,hsh3,CorrYlhsh3);//???????????????
    }
    eps.EpsZ=pow(Si0hsh3.SiVZ,2)+pow(Si0Yl.SiVZ,2)-2*Si0hsh3.SiVZ*Si0Yl.SiVZ*CorrYlhsh3.CorrZ+pow((V0hsh3.VZ-V0hsh3cl.VZ),2);
    if (eps.EpsZ!=0)
      eps.EpsZ=pow(eps.EpsZ,0.5);
    eps.EpsM=pow(Si0hsh3.SiVM,2)+pow(Si0Yl.SiVM,2)-2*Si0hsh3.SiVM*Si0Yl.SiVM*CorrYlhsh3.CorrM+pow((V0hsh3.VM-V0hsh3cl.VM),2);
    if (eps.EpsM!=0)
      eps.EpsM=pow(eps.EpsM,0.5);
    return eps;
}

OutData::V ParamsMatrix__4_4p::GetV0hsh1(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,  double hsh1, OutData::V V0hsh1d){
    OutData::VLayer V0hsh1;
    int M;
    if (InData::bottomLayer){
       if((hsh1>200) && (ProfRPV[0].height>=300)){
       //ан. 116, ф.135а
        V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_an(ProfRPV,hsh1).Vsr;//ан.116
       }
       else{
          //135
          paramsVdVsr_0_200m__3_3p1.GetV0layer(ProfRPV,hsh1,InData::V0ZM,V0hsh1d,M,V0hsh1.Vsr);
       }
    }
    else{
        if (hsh1>Constants::level200){
          if ((ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VZ!=Constants::UNDEF_V)&&
                (ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VM!=Constants::UNDEF_V)){
             V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_an(ProfRPV,hsh1).Vsr;//ан.116
          }
          else{
              //комбинации
              int ish1;
              FunctionsCalc::Poisk_hm(ProfRPV,hsh1,ish1);
              //V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hl(ProfRPV,ish1);
              V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2(ProfRPV,cl,hsh1).Vsr;
          }
        }
        else{
            //комбинации
            int ish1;
            FunctionsCalc::Poisk_hm(ProfRPV,hsh1,ish1);
           // V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hl(ProfRPV,ish1);
            V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2(ProfRPV,cl,hsh1).Vsr;
        }
    }
    return V0hsh1.Vsr;
}

OutData::V ParamsMatrix__4_4p::GetV0hsh1_4_4_7(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, double hsh1, OutData::V V0hsh1d){
    OutData::VLayer V0hsh1;
    int M;
    if (InData::bottomLayer){
       if((hsh1>200) && (ProfRPV[0].height>=300)){
       //ан. 116
        V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_an(ProfRPV/*,cl*/,hsh1).Vsr;//ан.116
       }
       else{
          //135
          paramsVdVsr_0_200m__3_3p1.GetV0layer(ProfRPV,hsh1,InData::V0ZM,V0hsh1d,M,V0hsh1.Vsr);
       }
    }
    else{
        if (hsh1>Constants::level200){
          if ((ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VZ!=Constants::UNDEF_V)&&
                (ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VM!=Constants::UNDEF_V)){
             V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_an(ProfRPV,hsh1).Vsr;//ан.116
          }
          else{
              //комбинации
              int ish1;
              FunctionsCalc::Poisk_hm(ProfRPV,hsh1,ish1);
              V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2(ProfRPV,cl,hsh1).Vsr;
          }
        }
        else{
            //комбинации
            int ish1;
            FunctionsCalc::Poisk_hm(ProfRPV,hsh1,ish1);
            V0hsh1.Vsr=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2(ProfRPV,cl,hsh1).Vsr;
        }
    }
    return V0hsh1.Vsr;
}

OutData::V ParamsMatrix__4_4p::GetV0h1sh(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int ihsh1, int ihsh2){
    OutData::VLayer V0hm;
    Calc_error::Eps Eps0hm;
    if(InData::k>=3)
        paramsVdVsr_Hl__4_1p1.Method0Hl(ProfRPV,cl,(ihsh1+1), V0hm.Vsr, Eps0hm);
    if(InData::k==2){
      FunctionsCalc_Layer::Get_VZ_VM_Layer_0_hm_0(ProfRPV,cl,ihsh1,ihsh2,V0hm);//ф.155
    }
    return V0hm.Vsr;
}

//ф.136, похоже на 116ан2
OutData::VLayer ParamsMatrix__4_4p::GetV0Hlsr_f136(PlowAlgoritm::ProfilePoint* ProfRPV, double hsh1, double Hl, OutData::V V0hsh1){
  OutData::VLayer V0Hlsr;
  double h00=0;
  if (InData::bottomLayer)
      h00=InData::h0;

  double Hlm1=hsh1;
  double h1=Hlm1;
  int iHl,iHlm1;
  FunctionsCalc::Poisk_hm(ProfRPV,Hl,iHl);
  iHlm1=0;
  FunctionsCalc::Poisk_hm(ProfRPV,hsh1,iHlm1);
  int M= iHl-iHlm1-1;
  int num=M+2;

  ParamsVdVsr_Hl__4_1p::L Lmas[num];
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHlm1].speed, ProfRPV[iHlm1].azimut,Lmas[0].Vi);
  Lmas[0].hi=h1;
  for (int i=(iHlm1+1);i<=iHl;i++){
      FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,Lmas[i-iHlm1].Vi);
      Lmas[i-iHlm1].hi= ProfRPV[i].height;
  }
  //------------------
  //!!!Для проверки
 /* std::cout<<"f136:"<<std::endl;
  std::cout<<"h"<<"\t"<<"VZ"<<"\t"<<"VM"<<"\t"<<std::endl;
  for(int i=0;i<num;i++)
      std::cout<<Lmas[i].hi<<"\t"<<Lmas[i].Vi.VZ<<"\t"<<Lmas[i].Vi.VM<<std::endl;*/
  //--------------------
  double sumZ=0, sumM=0;
  for (int n=0;n<=M;n++){
      sumZ+=(Lmas[n].Vi.VZ+Lmas[n+1].Vi.VZ)*(Lmas[n+1].hi-Lmas[n].hi);
      sumM+=(Lmas[n].Vi.VM+Lmas[n+1].Vi.VM)*(Lmas[n+1].hi-Lmas[n].hi);
  }
  V0Hlsr.Vsr.VZ=(V0hsh1.VZ*(h1-h00)+0.5*sumZ)/(Hl-h00);
  V0Hlsr.Vsr.VM=(V0hsh1.VM*(h1-h00)+0.5*sumM)/(Hl-h00);
  FunctionsCalc::Get_VZ_VM( ProfRPV[iHl].speed, ProfRPV[iHl].azimut,V0Hlsr.Vd);
  return V0Hlsr;
}

bool ParamsMatrix__4_4p::Check3lModel_4_4_3p(PlowAlgoritm::ProfilePoint* ProfRPV, int Nl, double Yl, int &j1, int &j2){
   j1=-1;j2=-1;
   double y1,y2;//134
   y1=Yl/3;
   y2=2*y1;
   double hmas[Nl];
   for (int i=0;i<Nl;i++)
       hmas[i]= ProfRPV[i].height;
   double hsh1;
   int k=0;
   int imin=0;
   while(k<Nl){
           hsh1=hsh(Nl, y1, hmas,imin);
           if (hsh1>=150)
               break;
           if (imin<0)
               break;
           hmas[imin]=-1;
           k++;
   }
   if(hsh1<150)
       return false;
   j1=imin;
   double hsh2=-1;
   for (int i=0;i<Nl;i++)
       hmas[i]= ProfRPV[i].height;
   k=0;
   while(k<Nl){
           hsh2=hsh(Nl, y2, hmas,imin);
           if (hsh2>hsh1)
               break;
           if (imin<0)
               break;
           hmas[imin]=-1;
           k++;
   }
   if (hsh2<=hsh1)
        return false;
   j2=imin;
   double SG=GetSG_443(y1, Yl, hsh1, hsh2);
   if ((SG/y1)>0.33) return false;
   return true;
}

double ParamsMatrix__4_4p::GetSG_443(double y1, double Yl, double hsh1,double hsh2){
    double SG;
    double h00=0;
    if (InData::bottomLayer)
        h00=InData::h0;
    SG=(pow((hsh1-h00-y1),2)+pow((hsh2-hsh1-y1),2)+pow((Yl-hsh2-y1),2))/2;
    SG=pow(SG,0.5);
    return SG;
}

void ParamsMatrix__4_4p::FormMatrix_4_4_6p(M **MZ,M **MM, int jnum, int inum){

    RankingM(MZ,jnum, inum);
    RankingM(MM,jnum, inum);
    ofstream efout;
    //Для проверки
    efout.open("M_nm.txt");

    efout<<"MZ.nm:"<<endl;
    for (int i=0;i<inum;i++){
        for (int j=0;j<jnum;j++) {
            efout<<MZ[j][i].nm<<"\t";
        }
        efout<<endl;
    }
    efout<<"MM.nm:"<<endl;
    for (int i=0;i<inum;i++){
        for (int j=0;j<jnum;j++) {
            efout<<MM[j][i].nm<<"\t";
        }
        efout<<endl;
    }
}

void ParamsMatrix__4_4p::RankingM(M **MX,int jnum, int inum){
    ParamsMatrix__4_4p::vectMod VM[jnum];
    double eps;
    int imod=0;
    for (int i=0;i<inum;i++){
        bool bi=true;
        for(int nj=0;nj<jnum;nj++)
           bi=bi&&(MX[nj][i].Eps==epsNoMod);
        if(bi){//если на уровне не было расчетов
          for(int nj=0;nj<jnum;nj++)
              MX[nj][i].nm=0;
           // VM[nj].iMod=0;
        }
        else{
           for (int j=0;j<jnum;j++){
            VM[j].epsMod=MX[j][i].Eps;
            VM[j].iMod=j;
           }
           for (int nj=0;nj<jnum;nj++){
            for(int ni=0;ni<(jnum-1);ni++){
               if (VM[ni+1].epsMod<VM[ni].epsMod){
                   eps=VM[ni].epsMod;
                   imod=VM[ni].iMod;
                   VM[ni].epsMod=VM[ni+1].epsMod;
                   VM[ni].iMod=VM[ni+1].iMod;
                   VM[ni+1].epsMod=eps;
                   VM[ni+1].iMod=imod;
               }
            }
           }
           for(int j=0;j<jnum;j++)
            MX[VM[j].iMod][i].nm=(j+1);
       }
    }
}

void ParamsMatrix__4_4p::Do4lModel_4_4_7p(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int j1, int j2,
                                            int l,double Yl,double Hl,OutData::VLayer V0Hl,int &NN,int iHl, int ihn,
                                            OutData::V **VCast,M **MZ, M **MM,double &HH,OutData::VLayer &V0HH){
    OutData::V delV0Yl0hsh2[7];
    Calc_error::Eps eps[7];
    for (int i=0;i<numMod;i++){
        eps[i].EpsZ=epsNoMod;
        eps[i].EpsM=epsNoMod;
    }

    double delhy=-1;
    OutData::VLayer V0h2sh3;
    GetVarMod_443(ProfRPV,cl,j1, j2,l, Yl,Hl, V0Hl,-1,VCast,delhy,V0h2sh3,delV0Yl0hsh2,eps);
    //-----------------------------------------------------------------
    int nl=NN-1;

    std::cout<<"Prirazheniya:"<<std::endl;
    for (int j=0;j<numMod;j++){
        MZ[j][0].delV=delV0Yl0hsh2[j].VZ;
        MZ[j][0].Eps=eps[j].EpsZ;
        MM[j][0].delV=delV0Yl0hsh2[j].VM;
        MM[j][0].Eps=eps[j].EpsM;
        //Для проверки
        std::cout<<j+1<<"\t"<<MZ[j][0].delV<<"\t\t"<<MZ[j][0].Eps<<"\t"<<MM[j][0].delV<<"\t\t"<<MM[j][0].Eps<<std::endl;
        //--------------
    }
    int Nl;
    bool m4,m3;
    int ii1,ii2,ii3;
    int N=1;//кол-во шагов, на которых могут быть отработаны все возможные модели приращений
    double hi=0;
    double delhyOut=-1;
    OutData::VLayer V0hi;
    OutData::V delV0hi0h2sh3;
    bool b1=((nl>0)&&(Hl>=1000));
    if(!b1) NN=1;//!!!posle
    if (b1)//если есть хоть 1 уровень ниже Yl
     for (int i=iHl;i>ihn;i--){
      hi= ProfRPV[i].height;
      Nl=GetNl_4_4_1p(ProfRPV,hi);
      if (Nl>2){
        Hl= ProfRPV[i-1].height;
        Get4lModel_4_4_2p(ProfRPV,hi,Nl,m4,m3,ii1,ii2,ii3);
        if (m3)
         if (Check3lModel_4_4_3p(ProfRPV, Nl, hi, ii1, ii2)){

             V0Hl=paramsVdVsr_Hl__4_1p1.GetV0Hlsr_l2_niz(ProfRPV,cl,Hl);
             GetVarMod_443(ProfRPV,cl,ii1, ii2,l, hi,Hl, V0Hl,delhy,VCast,delhyOut,V0h2sh3,delV0Yl0hsh2,eps);
             N++;
             FunctionsCalc::Get_VZ_VM( ProfRPV[i].speed, ProfRPV[i].azimut,V0hi.Vd);
             V0hi.Vsr=GetV0hsh1(ProfRPV,cl,hi,V0hi.Vd);
             delV0hi0h2sh3=GetdelV0hi0h2sh3(V0hi.Vsr,V0h2sh3.Vsr);
             for (int j=0;j<7;j++){
              if (delV0Yl0hsh2[j].VZ!=Constants::UNDEF_V){
              MZ[j][N-1].delV=delV0Yl0hsh2[j].VZ;
              MZ[j][N-1].Eps=fabs(delV0Yl0hsh2[j].VZ-delV0hi0h2sh3.VZ);
             }
              if (delV0Yl0hsh2[j].VM!=Constants::UNDEF_V){
             MM[j][N-1].delV=delV0Yl0hsh2[j].VM;
             MM[j][N-1].Eps=fabs(delV0Yl0hsh2[j].VM-delV0hi0h2sh3.VM);
            }
           }
        }

     }
    }
    NN=N;
    //---------------
    //!!!Для проверки
    ofstream ffout;
    ffout.open("M_delV.txt");

    ffout<<"MZ.delV:"<<endl;

        for (int i=0;i<NN;i++){
            for (int j=0;j<numMod;j++)
            ffout<<MZ[j][i].delV<<"\t";
        ffout<<endl;
    }
    ffout<<"MM.delV:"<<endl;
        for (int i=0;i<NN;i++){
            for (int j=0;j<numMod;j++)
            ffout<<MM[j][i].delV<<"\t";
        ffout<<endl;
    }
        ffout<<"N="<<N<<endl;
    ffout.close();

    ofstream efout;
    efout.open("M_eps.txt");

    efout<<"MZ.eps:"<<endl;

        for (int i=0;i<NN;i++){
            for (int j=0;j<numMod;j++)
            efout<<MZ[j][i].Eps<<"\t";
        efout<<endl;
    }
    efout<<"MM.eps:"<<endl;
        for (int i=0;i<NN;i++){
            for (int j=0;j<numMod;j++)
            efout<<MM[j][i].Eps<<"\t";
        efout<<endl;
    }
        efout<<"N="<<N<<endl;
    efout.close();
    //-----------------
    HH=Hl;
    V0HH=V0Hl;
    ParamsdelV__4_3p::IPRHH=1;
}

void ParamsMatrix__4_4p::GetVarMod_443(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int j1, int j2,
                                       int l,double Yl,double Hl,OutData::VLayer V0Hl,
                                       double delhyIn,OutData::V** VCast,double &delhyOut, OutData::VLayer &V0hsh2 ,OutData::V *delV0Yl0hsh2,Calc_error::Eps *eps){
    delhyOut=-1;
    double hsh1= ProfRPV[j1].height;
    double hsh2= ProfRPV[j2].height;
    OutData::VLayer V0hsh1;
    FunctionsCalc::Get_VZ_VM( ProfRPV[j1].speed, ProfRPV[j1].azimut,V0hsh1.Vd);
    FunctionsCalc::Get_VZ_VM( ProfRPV[j2].speed, ProfRPV[j2].azimut,V0hsh2.Vd);
    V0hsh1.Vsr=GetV0hsh1_4_4_7(ProfRPV,cl,hsh1,V0hsh1.Vd);
    V0hsh2.Vsr=GetV0Hlsr_f136(ProfRPV,hsh1,hsh2,V0hsh1.Vsr).Vsr;//136
    if (delhyIn<0){
        //Переопределение параметров в конце блока 4.4.7 c.163??????????????
        ParamsVd_Yl__4p::HH=ParamsVd_Yl__4p::Hlmas[l-1];
        ParamsVd_Yl__4p::V0HH=ParamsVd_Yl__4p::V0Hl[l-1];
        ParamsVd_Yl__4p::Hlmas[l-1]=hsh2;
        ParamsVd_Yl__4p::V0Hl[l-1]=V0hsh2;
    }
    ClimatData::Pr pr0hsh1cl,pr0hsh2cl;
    OutData::V V0hsh1cl,V0hsh2cl;
    pr0hsh1cl=GetV0hshcl(cl,hsh1);
    V0hsh1cl.VZ=pr0hsh1cl.VZ;V0hsh1cl.VM=pr0hsh1cl.VM;
    pr0hsh2cl=GetV0hshcl(cl,hsh2);
    V0hsh2cl.VZ=pr0hsh2cl.VZ;V0hsh2cl.VM=pr0hsh2cl.VM;
    OutData::V delVrcl21;
    delVrcl21=GetdelVrcl(V0hsh2.Vsr,V0hsh1.Vsr,V0hsh2cl,V0hsh1cl);//138

    double h00=0;
    if(InData::bottomLayer)
        h00=InData::h0;
    double delh1, delh2,delh3;
    delh1=hsh1-h00;
    delh2=hsh2-hsh1;
    delh3=Yl-hsh2;

    ClimatData::Pr V0Ylcl;
    OutData::V delV0Yl0hsh2cl;
    int im;
    int Kmod=0;

    if (delhyIn<0){
        Climat::Get_Vcl_h(cl,Yl,V0Ylcl);
        delV0Yl0hsh2cl.VZ=V0Ylcl.VZ-V0hsh2cl.VZ;
        delV0Yl0hsh2cl.VM=V0Ylcl.VM-V0hsh2cl.VM;

        delV0Yl0hsh2[0]=GetdelV0Yl0hsh2_1v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[0]);
        delV0Yl0hsh2[1]=GetdelV0Yl0hsh2_2v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[1]);
        delV0Yl0hsh2[2]=GetdelV0Yl0hsh2_3v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[2]);
        delV0Yl0hsh2[3]=GetdelV0Yl0hsh2_4v(cl,Yl,hsh2,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[3]);
        delV0Yl0hsh2[4]=GetdelV0Yl0hsh3_5v(cl,Yl,hsh2,delV0Yl0hsh2cl,V0hsh2.Vsr,V0hsh2cl,eps[4]);
        int Kmod=5;
        if (FunctionsCalc::Poisk_hm_ontop(ProfRPV,Yl,im))
           if(ProfRPV[im].curr==1){
            delV0Yl0hsh2[5]=GetdelV0Yl0hsh3_6v(ProfRPV,cl,im,Yl,Hl,h00,V0Hl,V0hsh2.Vsr,eps[5],delhyOut);
            Kmod++;
           }
         if(delhyOut<0)delhyOut=0;
        }
        else{//для нижележащих уровней
           V0Ylcl=GetV0hshcl(cl,Yl);
           delV0Yl0hsh2cl.VZ=V0Ylcl.VZ-V0hsh2cl.VZ;
           delV0Yl0hsh2cl.VM=V0Ylcl.VM-V0hsh2cl.VM;
           delV0Yl0hsh2[0]=GetdelV0Yl0hsh2_1v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[0]);
           delV0Yl0hsh2[1]=GetdelV0Yl0hsh2_2v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[1]);
           delV0Yl0hsh2[2]=GetdelV0Yl0hsh2_3v(cl,Yl,hsh2,delh1,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[2]);
           delV0Yl0hsh2[3]=GetdelV0Yl0hsh2_4v(cl,Yl,hsh2,delh2,delh3,delV0Yl0hsh2cl,delVrcl21,eps[3]);
           delV0Yl0hsh2[4]=GetdelV0Yl0hsh3_5v(cl,Yl,hsh2,delV0Yl0hsh2cl,V0hsh2.Vsr,V0hsh2cl,eps[4]);
           Kmod=5;
           if(delhyIn>0){
            int imsh=FunctionsCalc::Poisk_imsh(ProfRPV,Yl+delhyIn);
            delV0Yl0hsh2[5]=GetdelV0Yl0hsh3_6v(ProfRPV,cl,imsh,Yl,Hl,h00,V0Hl,V0hsh2.Vsr,eps[5],delhyOut);
            Kmod++;
           }
     }
    if (InData::prevZond){
        delV0Yl0hsh2[6]=GetdelV0Yl0hsh3_7v(cl,Yl,hsh2,V0hsh2.Vsr,VCast,eps[6]);
        Kmod++;
    }
}

//156
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh2_1v(const ClimatData* cl,double Yl, double hsh2, double delh1, double delh2,double delh3,
                                                  OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1){
    OutData::V delV0Yl0hsh2;
    delV0Yl0hsh2.VZ=delV0Yl0hsh2cl.VZ+delVrcl21.VZ*delh3*delh1/(delh2*(delh1+delh2+delh3));//
    delV0Yl0hsh2.VM=delV0Yl0hsh2cl.VM+delVrcl21.VM*delh3*delh1/(delh2*(delh1+delh2+delh3));
    Eps1=GetEps(cl,Yl,hsh2,delV0Yl0hsh2,delV0Yl0hsh2cl);
    return delV0Yl0hsh2;
}

//157
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh2_2v(const ClimatData* cl,double Yl, double hsh2, double delh1, double delh2,double delh3,
                                                  OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1){
    OutData::V delV0Yl0hsh2;
    delV0Yl0hsh2.VZ=delV0Yl0hsh2cl.VZ+delVrcl21.VZ*delh3*(delh1/delh2+1)/(delh1+delh2+delh3);//
    delV0Yl0hsh2.VM=delV0Yl0hsh2cl.VM+delVrcl21.VM*delh3*(delh1/delh2+1)/(delh1+delh2+delh3);
    Eps1=GetEps(cl,Yl,hsh2,delV0Yl0hsh2,delV0Yl0hsh2cl);
    return delV0Yl0hsh2;
}

//158
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh2_3v(const ClimatData* cl,double Yl, double hsh2, double delh1, double delh2,double delh3,
                                                  OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1){
    OutData::V delV0Yl0hsh2;
    delV0Yl0hsh2.VZ=delV0Yl0hsh2cl.VZ+delVrcl21.VZ*delh3*(2*delh1/delh2+1)/(2*(delh1+delh2+delh3));//
    delV0Yl0hsh2.VM=delV0Yl0hsh2cl.VM+delVrcl21.VM*delh3*(2*delh1/delh2+1)/(2*(delh1+delh2+delh3));
    Eps1=GetEps(cl,Yl,hsh2,delV0Yl0hsh2,delV0Yl0hsh2cl);
    return delV0Yl0hsh2;
}

//159
OutData::V ParamsMatrix__4_4p::GetdelV0Yl0hsh2_4v(const ClimatData* cl,double Yl, double hsh2, double delh2,double delh3,
                                                  OutData::V delV0Yl0hsh2cl, OutData::V delVrcl21, Calc_error::Eps &Eps1){
    OutData::V delV0Yl0hsh2;
    delV0Yl0hsh2.VZ=delV0Yl0hsh2cl.VZ+delVrcl21.VZ*delh3/delh2;//
    delV0Yl0hsh2.VM=delV0Yl0hsh2cl.VM+delVrcl21.VM*delh3/delh2;
    Eps1=GetEps(cl,Yl,hsh2,delV0Yl0hsh2,delV0Yl0hsh2cl);
    return delV0Yl0hsh2;
}

void ParamsMatrix__4_4p::GetDel_4_4_5p(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int l,double Yl,double Hl,
                                       OutData::V V0Hlsr,OutData::V **VCast,int &NN, M **MZ, M **MM){
    NN=1;
    for (int i=0;i<3;i++){
      paramsdelV__4_3p1.ComplMode[i].Eps.EpsZ=ParamsMatrix__4_4p::epsNoMod;
      paramsdelV__4_3p1.ComplMode[i].Eps.EpsM=ParamsMatrix__4_4p::epsNoMod;
    }
    paramsdelV__4_3p1.ComplMode[0].delV0Yl0Hl=paramsdelV__4_3p1.GetdelV_1v(cl,Yl,Hl,V0Hlsr,paramsdelV__4_3p1.ComplMode[0].Eps);
    paramsdelV__4_3p1.ComplMode[1].delV0Yl0Hl=paramsdelV__4_3p1.GetdelV_2v(ProfRPV,cl,l,Yl,Hl,paramsdelV__4_3p1.ComplMode[1].Eps);
    paramsdelV__4_3p1.ComplMode[2].delV0Yl0Hl=GetdelV_3v(cl,l,Hl,V0Hlsr,VCast,paramsdelV__4_3p1.ComplMode[2].Eps);//разница есть предыдущ. зондирования
    paramsdelV__4_3p1.Getkmod();
    for (int j=0;j<3;j++){
        MZ[j][0].delV=paramsdelV__4_3p1.ComplMode[j].delV0Yl0Hl.VZ;
        MZ[j][0].Eps=paramsdelV__4_3p1.ComplMode[j].Eps.EpsZ;
        MM[j][0].delV=paramsdelV__4_3p1.ComplMode[j].delV0Yl0Hl.VM;
        MM[j][0].Eps=paramsdelV__4_3p1.ComplMode[j].Eps.EpsM;
    }
    ParamsdelV__4_3p::IPRHH=0;
}

OutData::V ParamsMatrix__4_4p::GetdelV_3v(const ClimatData* cl, int l, double Hl,
                                          OutData::V V0Hlsr,OutData::V **VCast,Calc_error::Eps &Eps3){
   OutData::V delV0Yl0Hl;
   //Если отсутствуют предыдущие зондирования
   if (!InData::prevZond)
       return delV0Yl0Hl;
   GetNumMB_H__2_5p(cl,Hl,l,V0Hlsr,VCast,delV0Yl0Hl,Eps3) ;
   return delV0Yl0Hl;
}

OutData::V ParamsMatrix__4_4p::GetdelV_7v(const ClimatData* cl,double Yl, double Hl,
                                          OutData::V V0Hlsr,OutData::V **VCast,Calc_error::Eps &Eps3){
   OutData::V delV0Yl0Hl;
   //Если отсутствуют предыдущие зондирования
   if (!InData::prevZond)
       return delV0Yl0Hl;
   GetNumMB_H7v__2_5p(cl,Yl,Hl/*,l*/,V0Hlsr,VCast,delV0Yl0Hl,Eps3) ;
   return delV0Yl0Hl;
}

//Определяет номер метеобюллетеня для восстановления параметров
//среднего ветра в слояях выше 0-200
//V0Hlsr-скорость среднего ветра в слое 0-Hl,
//рассчитанная в п.4.1
int ParamsMatrix__4_4p::GetNumMB_H__2_5p(const ClimatData* cl, double Hl,int l,OutData::V V0Hlsr,OutData::V **VCast,
                                       OutData::V &delV_3v, Calc_error::Eps &Eps3){
    int numMB_H=-1;
    double _V0Hlsr, TETA0Hlsr;
    double V0Hlj, TETA0Hlj;
    double Yl=Constants::StandartLayer[l-1];
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
    bool b1=false,b2=false,b3=false;
    OutData::V V0Hltau[InData::kprev];
    OutData::V V0Yltau[InData::kprev];
    int m,mpl1,n,npl1;
    FunctionsCalc::PoiskBoundsLevelsMB(Hl,m,mpl1);
    FunctionsCalc::PoiskBoundsLevelsMB(Yl,n,npl1);
    for(int k=0;k<InData::kprev;k++){
         b1=((VCast[k][m].VZ!=Constants::UNDEF_V)&&(VCast[k][m].VM!=Constants::UNDEF_V));
         b2=((VCast[k][mpl1].VZ!=Constants::UNDEF_V)&&(VCast[k][mpl1].VM!=Constants::UNDEF_V));
         b3=((VCast[k][n].VZ!=Constants::UNDEF_V)&&(VCast[k][n].VM!=Constants::UNDEF_V)&&
             (VCast[k][npl1].VZ!=Constants::UNDEF_V)&&(VCast[k][npl1].VM!=Constants::UNDEF_V));
         if(b1 && b2 && b3){
           CHM=InData::GetCHfromDate(InData::MBulletin_table[k].tauM.GD0,InData::MBulletin_table[k].tauM.MS0,
                                   InData::MBulletin_table[k].tauM.DN0,InData::MBulletin_table[k].tauM.CH0);
           deltau[k]=CH0-CHM;
           if(Hl<=Constants::level200)
               V0Hltau[k]=VCast[k][0];
           else{

               FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),m,mpl1,Hl,VCast,V0Hltau[k]);
           }
          FunctionsCalc::Get_VZ_VM_inv(V0Hltau[k],V0Hlj,TETA0Hlj);
          delV[k]=bottomEval__2p4.GetdelV(_V0Hlsr, TETA0Hlsr,V0Hlj,TETA0Hlj);//ф.124

          FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),n,npl1,Yl,VCast,V0Yltau[k]);

         }
      }

    MathFunc::Rank R_deltau[InData::kprev];
    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
    MathFunc::Rank R_delV[InData::kprev];
    MathFunc::RankingIncrMas1(delV,InData::kprev,R_delV);

    int M[2][InData::kprev];
    for (int k=0;k<InData::kprev;k++){//Заполняем массив мест
        M[0][k]=bottomEval__2p4.GetM(R_deltau,InData::kprev,k);
        M[1][k]=bottomEval__2p4.GetM(R_delV,InData::kprev,k);
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
      double mindeltau=deltau[R_sum[0].ind];
      for(int k=1;k<=eqsum;k++)
          if (deltau[R_sum[k].ind]<mindeltau){
             numMB_H=R_sum[k].ind+1;
             mindeltau=deltau[R_sum[k].ind];
          }
    }
    bool undeff=false;
    bool sumundeff=true;
    for(int lmb=1;lmb<Constants::numStL_MB/*Climat::numStL*/;lmb++){
        undeff=((InData::MBulletin_table[numMB_H-1].ProfMeteo[lmb].V==Constants::UNDEF_MeteoB)||
                (InData::MBulletin_table[numMB_H-1].ProfMeteo[lmb].V==Constants::UNDEF_V));
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
      Eps3=bottomEval__2p4.Eps_2v(cl,Yl/*?????*/, Hl,deltau[numMB_H-1],V0Hlsr,V0Hltau[numMB_H-1]);//ф.127
    }
    return numMB_H;
}

//Определяет номер метеобюллетеня для восстановления параметров
//среднего ветра в слояях выше 0-200
//V0Hlsr-скорость среднего ветра в слое 0-Hl,
//рассчитанная в п.4.1
int ParamsMatrix__4_4p::GetNumMB_H7v__2_5p(const ClimatData* cl,double Yl,double Hl,/*int l,*/OutData::V V0Hlsr,OutData::V **VCast,
                                       OutData::V &delV_3v, Calc_error::Eps &Eps3){
  //  int is=Climat::Get_is(Yl);
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
    bool b1=false,b2=false,b3=false;
    OutData::V V0Hltau[InData::kprev];
    OutData::V V0Yltau[InData::kprev];
    int m,mpl1,n,npl1;
    FunctionsCalc::PoiskBoundsLevelsMB(Hl,m,mpl1);
    FunctionsCalc::PoiskBoundsLevelsMB(Yl,n,npl1);
    for(int k=0;k<InData::kprev;k++){
         b1=((VCast[k][m].VZ!=Constants::UNDEF_V)&&(VCast[k][m].VM!=Constants::UNDEF_V));
         b2=((VCast[k][mpl1].VZ!=Constants::UNDEF_V)&&(VCast[k][mpl1].VM!=Constants::UNDEF_V));
         b3=((VCast[k][n].VZ!=Constants::UNDEF_V)&&(VCast[k][n].VM!=Constants::UNDEF_V)&&
             (VCast[k][npl1].VZ!=Constants::UNDEF_V)&&(VCast[k][npl1].VM!=Constants::UNDEF_V));
         if(b1 && b2 && b3){
           CHM=InData::GetCHfromDate(InData::MBulletin_table[k].tauM.GD0,InData::MBulletin_table[k].tauM.MS0,
                                   InData::MBulletin_table[k].tauM.DN0,InData::MBulletin_table[k].tauM.CH0);
           deltau[k]=CH0-CHM;
           if(Hl<=Constants::level200)
               V0Hltau[k]=VCast[k][0];
           else{

               FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),m,mpl1,Hl,VCast,V0Hltau[k]);
           }
          FunctionsCalc::Get_VZ_VM_inv(V0Hltau[k],V0Hlj,TETA0Hlj);
          delV[k]=bottomEval__2p4.GetdelV(_V0Hlsr, TETA0Hlsr,V0Hlj,TETA0Hlj);//ф.124

          FunctionsCalc::GetVTeta_LineInterpol2_MB((k+1),n,npl1,Yl,VCast,V0Yltau[k]);
         }
      }

    MathFunc::Rank R_deltau[InData::kprev];
    MathFunc::RankingIncrMas1(deltau,InData::kprev,R_deltau);
    MathFunc::Rank R_delV[InData::kprev];
    MathFunc::RankingIncrMas1(delV,InData::kprev,R_delV);

    int M[2][InData::kprev];
    for (int k=0;k<InData::kprev;k++){//Заполняем массив мест
        M[0][k]=bottomEval__2p4.GetM(R_deltau,InData::kprev,k);
        M[1][k]=bottomEval__2p4.GetM(R_delV,InData::kprev,k);
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
      double mindeltau=deltau[R_sum[0].ind];
      for(int k=1;k<=eqsum;k++)
          if (deltau[R_sum[k].ind]<mindeltau){
             numMB_H=R_sum[k].ind+1;
             mindeltau=deltau[R_sum[k].ind];
          }
    }
    bool undeff=false;
    bool sumundeff=true;
    for(int lmb=1;lmb<Constants::numStL_MB/*Climat::numStL*/;lmb++){
        undeff=((InData::MBulletin_table[numMB_H-1].ProfMeteo[lmb].V==Constants::UNDEF_MeteoB)||
                (InData::MBulletin_table[numMB_H-1].ProfMeteo[lmb].V==Constants::UNDEF_V));
        sumundeff=sumundeff && undeff;
    }
    if (sumundeff==true)
        return -1;
    if ((InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_MeteoB)||
            (InData::MBulletin_table[numMB_H-1].ProfMeteo[0].V==Constants::UNDEF_V))
        return -1;

    if((V0Yltau[numMB_H-1].VZ!=Constants::UNDEF_V) && (V0Yltau[numMB_H-1].VM!=Constants::UNDEF_V) &&
            (V0Hltau[numMB_H-1].VZ!=Constants::UNDEF_V) && (V0Hltau[numMB_H-1].VM!=Constants::UNDEF_V))
    {
     delV_3v.VZ=V0Yltau[numMB_H-1].VZ-V0Hltau[numMB_H-1].VZ;//ф.126
     delV_3v.VM=V0Yltau[numMB_H-1].VM-V0Hltau[numMB_H-1].VM;

     //int ih=FunctionsCalc::Poisk_imsh_low(Yl);
     Eps3=bottomEval__2p4.Eps_2v(cl,/*(ih+1)*/Yl, Hl,deltau[numMB_H-1],V0Hlsr,V0Hltau[numMB_H-1]);//ф.127
    }
    return numMB_H;
}








