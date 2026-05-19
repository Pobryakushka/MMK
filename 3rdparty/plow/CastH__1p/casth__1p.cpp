#include "casth__1p.h"
#include "InData/InData.h"
#include "FunctionsCalc/functionscalc.h"
#include "math.h"
#include "InData/indataclimat.h"

#include <iostream>
#include "stdlib.h"
#include "stdio.h"

using namespace std;

CastH__1p::CastH__1p()
{
}

//Расчет приведенных значений скорости
void CastH__1p::GetVCast(const ClimatData* cl,OutData::V **VCast ){
    //Массив значений скорости из МБ, пересчитанных в
    //зональную и меридиальную составляющие
    OutData::V **VMeteoB;
    VMeteoB=new OutData::V * [InData::kprev];
    for (int i=0;i<InData::kprev;i++){
       VMeteoB[i]=new OutData::V[Constants::numStL_MB];
    }
    GetMasVBM(VMeteoB);
    //----------------
   /* cout<<"Meteo ZM:"<<endl;
    for (int k=2;k<3;k++){
        for (int lev=0;lev<Constants::numStL;lev++){
            cout<<InData::MBulletin_table[k].ProfMeteo[lev].V<<" "<<InData::MBulletin_table[k].ProfMeteo[lev].TETA<<" "
                <<VMeteoB[k][lev].VZ<<" "<<VMeteoB[k][lev].VM<<endl;
        }
    }*/
    //----------------
    for (int k=0;k<InData::kprev;k++){
        for (int lev=0;lev<Constants::numStL_MB;lev++){
            {
            if (InData::MBulletin_table[k].Hm==Constants::UNDEF_MeteoB)
                VCast[k][lev]=VMeteoB[k][lev];
            else{
              if (abs(InData::MBulletin_table[k].Hm-InData::H)<=Constants::level100)
                VCast[k][lev]=VMeteoB[k][lev];
              else  VCast[k][lev]=ProcCast(cl,k,lev,VMeteoB);
            }
          }
        }
    }
    for (int i=0;i<InData::kprev;i++) {
        delete [] VMeteoB[i];
    }
    delete [] VMeteoB;
}

//Пересчет значений скорости из МБ в зон. и мер. составляющие
void CastH__1p::GetMasVBM(OutData::V **VMeteoB){
    double VMB;
    double TETAMB;
    for (int k=0;k<InData::kprev;k++){
        for (int lev=0;lev<Constants::numStL_MB/*Constants::numStL*/;lev++){
            VMB=InData::MBulletin_table[k].ProfMeteo[lev].V;
            TETAMB=InData::MBulletin_table[k].ProfMeteo[lev].TETA;
            if (VMB==Constants::UNDEF_MeteoB){
                VMeteoB[k][lev].VZ=Constants::UNDEF_V;
                VMeteoB[k][lev].VM=Constants::UNDEF_V;
            }
            else
              FunctionsCalc::Get_VZ_VM(VMB,TETAMB,VMeteoB[k][lev]);
         std::cout<<VMeteoB[k][lev].VZ<<" "<<VMeteoB[k][lev].VM<<" "<<std::endl;
        }
      std::cout<<"\r\n";
     }
}

//Процедура приведения
OutData::V CastH__1p::ProcCast(const ClimatData* cl,int k,int lev,OutData::V **VMeteoB){
   OutData::V VCast;
   if (InData::MBulletin_table[k].Hm>InData::H)
       VCast=ProcCast_1v(cl,k,lev,VMeteoB);
   else if (InData::MBulletin_table[k].Hm<InData::H)
       VCast=ProcCast_2v(cl,k,lev,VMeteoB);
   return VCast;
}

OutData::V CastH__1p::ProcCast_1v(const ClimatData* cl, int k, int lev,OutData::V **VMeteoB){
   OutData::V VCast;
    //ф.3
   double Hm=InData::MBulletin_table[k].Hm;//высота станции зондирования
   double H=InData::H;
   double Yl=Constants::StandartLayerMB[lev];
   double h1=Hm-H;
   double h2=h1+Yl;
   double h3=h2+Hm-H;
   double h=Yl;
   if (Yl>=(Hm-H+Constants::level200)&&
        (VMeteoB[k][lev].VZ==Constants::UNDEF_V)&&(VMeteoB[k][lev].VM==Constants::UNDEF_V)){
       h2=Yl;
       h3=h2+Hm-H;
       h=Yl-Hm+H;
   }
   OutData::V Vclh1h2=GetVclh1h2_f4(cl,h1, h2);
   ClimatData::Pr Vcl0h3;
   if (h3<=Constants::StandartLayer[Constants::numStL-1])
           Climat::Get_Vcl_h(cl,h3,Vcl0h3);
   OutData::V V0h;
   if (h==Yl){
       V0h=VMeteoB[k][lev];
       if ((VMeteoB[k][lev].VZ==Constants::UNDEF_V)&&(VMeteoB[k][lev].VM==Constants::UNDEF_V)){
           VCast=VMeteoB[k][lev];
           return VCast;
       }
   }
   if (h==(Yl-Hm+H)){
       V0h=GetV0h_f10(h, k, VMeteoB);
   }
   if ((V0h.VZ==Constants::UNDEF_V) &&
           (V0h.VM==Constants::UNDEF_V)){
       VCast=V0h;
       return VCast;
   }
   /*OutData::V*/ClimatData::Pr Vcl0Yl;
   //!pr Climat::Get_Vcl(Climat::StandartLayer[lev],Climat::Zone,Vcl0Yl);
   Vcl0Yl=cl->pr(Constants::StandartLayerMB[lev]);
   if (h3<=Constants::level8000){
       VCast.VZ=V0h.VZ-0.5*(Vclh1h2.VZ+Vcl0h3.VZ)+Vcl0Yl.VZ;//ф.8
       VCast.VM=V0h.VM-0.5*(Vclh1h2.VM+Vcl0h3.VM)+Vcl0Yl.VM;
   }
   else{
       VCast.VZ=V0h.VZ-Vclh1h2.VZ+Vcl0Yl.VZ;//ф.9
       VCast.VM=V0h.VM-Vclh1h2.VM+Vcl0Yl.VM;
   }

   return VCast;
}

//ф.4
OutData::V CastH__1p::GetVclh1h2_f4(const ClimatData* cl,double h1, double h2){
    OutData::V Vclh1h2;
    ClimatData::Pr Vcl0h1,Vcl0h2;
    if (h1<Constants::level200)
       //!pr Climat::Get_Vcl(Constants::level200,Climat::Zone,Vcl0h1);
        Vcl0h1=cl->pr(Constants::level200);
    if (h1>Climat::StandartLayer[Constants::numStL-1])
       //!pr Climat::Get_Vcl(Constants::level8000,Climat::Zone,Vcl0h1);
        Vcl0h1=cl->pr(Constants::level8000);
    if ((h1>=Constants::level200)&&(h1<=Constants::level8000))
        Climat::Get_Vcl_h(cl,h1,Vcl0h1);
    if (h2>Climat::StandartLayer[Constants::numStL-1])
        //!pr Climat::Get_Vcl(Constants::level8000,Climat::Zone,Vcl0h2);
        Vcl0h2=cl->pr(Constants::level8000);
    if ((h2>=Constants::level200)&&(h2<=Constants::level8000))
        Climat::Get_Vcl_h(cl,h2,Vcl0h2);

    Vclh1h2.VZ=(Vcl0h2.VZ*h2-Vcl0h1.VZ*h1)/(h2-h1);
    Vclh1h2.VM=(Vcl0h2.VM*h2-Vcl0h1.VM*h1)/(h2-h1);
    return Vclh1h2;
}

//ф.10
OutData::V CastH__1p::GetV0h_f10(double h, int k, OutData::V **VMeteoB){
   OutData::V V0h;
   int is=Climat::Get_is_MB(h);
   //Если h - стандартный слой
   if (is>0)
      V0h=VMeteoB[k][is];
   else{
       int i2sh=Climat::Get_is_h_MB(h);
       bool b1=(VMeteoB[k][i2sh].VZ==Constants::UNDEF_V) &&
               (VMeteoB[k][i2sh].VM==Constants::UNDEF_V);
       bool b2=(VMeteoB[k][i2sh-1].VZ==Constants::UNDEF_V) &&
               (VMeteoB[k][i2sh-1].VM==Constants::UNDEF_V);
       if (b1||b2){
          V0h.VZ=Constants::UNDEF_V;
          V0h.VM=Constants::UNDEF_V;
          return V0h;
       }
       double h2sh=Constants::StandartLayerMB[i2sh];
       double hsh=Constants::StandartLayerMB[i2sh-1];
       OutData::V V0hsh,V0h2sh;
       V0hsh=VMeteoB[k][i2sh-1];
       V0h2sh=VMeteoB[k][i2sh];
       V0h.VZ=V0hsh.VZ+(V0h2sh.VZ-V0hsh.VZ)/(h2sh-hsh)*(h-hsh);
       V0h.VM=V0hsh.VM+(V0h2sh.VM-V0hsh.VM)/(h2sh-hsh)*(h-hsh);
   }

   return V0h;
}

OutData::V CastH__1p::ProcCast_2v(const ClimatData* cl, int k,int lev,OutData::V **VMeteoB){
    OutData::V VCast;
    //ф.11
   double Hm=InData::MBulletin_table[k].Hm;//высота станции зондирования
   double H=InData::H;
   double Yl=Constants::StandartLayerMB[lev];
   double h1=H-Hm;
   double h2=H-Hm+Yl;
   OutData::V V0h1, V0h2;
   if (h1>=Constants::level200){
       V0h1=GetV0h_f12(cl,h1, k, VMeteoB);//ф.12
       if ((V0h1.VZ==Constants::UNDEF_V)&&(V0h1.VM==Constants::UNDEF_V)){
          VCast=V0h1;
          return VCast;
       }
   }
   else{
     V0h1=GetV0h1_f13(cl,h1,k,VMeteoB);
     if ((V0h1.VZ==Constants::UNDEF_V)&&(V0h1.VM==Constants::UNDEF_V)){
        VCast=V0h1;
        return VCast;
     }
   }
   V0h2=GetV0h_f12(cl,h2, k, VMeteoB);//ф.17 (ф.12)
   if ((V0h2.VZ==Constants::UNDEF_V)&&(V0h2.VM==Constants::UNDEF_V)){
      VCast=V0h2;
      return VCast;
   }
   VCast.VZ=(V0h2.VZ*h2-V0h1.VZ*h1)/(h2-h1);//ф.18
   VCast.VM=(V0h2.VM*h2-V0h1.VM*h1)/(h2-h1);
   return VCast;
}

//ф.12
OutData::V CastH__1p::GetV0h_f12(const ClimatData* cl, double h, int k, OutData::V **VMeteoB){
   OutData::V V0h;
   int is=Climat::Get_is_MB(h);
   //Если h - стандартный слой
   if (is>0)
      V0h=VMeteoB[k][is];
   else{
       int i2sh=Climat::Get_is_h_MB(h);
       if (i2sh<0) return V0h;
       bool b1=(VMeteoB[k][i2sh].VZ==Constants::UNDEF_V) &&
               (VMeteoB[k][i2sh].VM==Constants::UNDEF_V);
       bool b2=(VMeteoB[k][i2sh-1].VZ==Constants::UNDEF_V) &&
               (VMeteoB[k][i2sh-1].VM==Constants::UNDEF_V);
       if (b1||b2){
          V0h.VZ=Constants::UNDEF_V;
          V0h.VM=Constants::UNDEF_V;
          return V0h;
       }
       double h2sh=Constants::StandartLayerMB[i2sh];
       double hsh=Constants::StandartLayerMB[i2sh-1];
       OutData::V V0hsh,V0h2sh;
       if ((hsh==Constants::level200)&&
        (VMeteoB[k][0].VZ==Constants::UNDEF_V) && (VMeteoB[k][0].VZ==Constants::UNDEF_V)){
           if((VMeteoB[k][1].VZ!=Constants::UNDEF_V) && (VMeteoB[k][1].VZ!=Constants::UNDEF_V)){
              ClimatData::Pr Vcl0200, Vcl0400;
              //!pr Climat::Get_Vcl(Constants::level200,Climat::Zone,Vcl0200);
              Vcl0200=cl->pr(Constants::level200);
              //!pr Climat::Get_Vcl(Constants::level400,Climat::Zone,Vcl0400);
              Vcl0400=cl->pr(Constants::level400);
              V0h.VZ=VMeteoB[k][1].VZ-Vcl0400.VZ+Vcl0200.VZ;//ф.16
              V0h.VM=VMeteoB[k][1].VM-Vcl0400.VM+Vcl0200.VM;
           }
          return V0h;
       }
       V0hsh=VMeteoB[k][i2sh-1];
       V0h2sh=VMeteoB[k][i2sh];
       if (((V0hsh.VZ==Constants::UNDEF_V)&&(V0hsh.VM==Constants::UNDEF_V))
          ||((V0h2sh.VZ==Constants::UNDEF_V)&&(V0h2sh.VM==Constants::UNDEF_V)))
           return V0h;
       V0h.VZ=V0hsh.VZ+(V0h2sh.VZ-V0hsh.VZ)/(h2sh-hsh)*(h-hsh);
       V0h.VM=V0hsh.VM+(V0h2sh.VM-V0hsh.VM)/(h2sh-hsh)*(h-hsh);
   }
   return V0h;
}

//ф.13
OutData::V CastH__1p::GetV0h1_f13(const ClimatData* cl, double h1, int k, OutData::V **VMeteoB){
    OutData::V V0h1;
    /*OutData::V*/ClimatData::Pr V0200cl,V0h1cl;
    //!pr Climat::Get_Vcl(Constants::level200, Climat::Zone, V0200cl);
    V0200cl=cl->pr(Constants::level200);
    V0h1cl=GetV0h1cl_f14(h1, V0200cl);
    bool b200=(VMeteoB[k][0].VZ==Constants::UNDEF_V)&&(VMeteoB[k][0].VM==Constants::UNDEF_V);
    bool b400=(VMeteoB[k][1].VZ==Constants::UNDEF_V)&&(VMeteoB[k][1].VM==Constants::UNDEF_V);
    if (b200 && b400){
       V0h1.VZ=Constants::UNDEF_V;
       V0h1.VM=Constants::UNDEF_V;
       return V0h1;
    }
    if (b200 && (!b400)){
        ClimatData::Pr V0400cl;
        //!pr Climat::Get_Vcl(Constants::level400,Climat::Zone,V0400cl);
        V0400cl=cl->pr(Constants::level400);
        V0h1.VZ=VMeteoB[k][1].VZ-V0400cl.VZ+V0h1cl.VZ;//ф.15
        V0h1.VM=VMeteoB[k][1].VM-V0400cl.VM+V0h1cl.VZ;
        return V0h1;
    }
    V0h1.VZ=VMeteoB[k][0].VZ-V0200cl.VZ+V0h1cl.VZ;//ф.13
    V0h1.VM=VMeteoB[k][0].VM-V0200cl.VM+V0h1cl.VM;
    return V0h1;
}

//ф.14
ClimatData::Pr CastH__1p::GetV0h1cl_f14(double h1,ClimatData::Pr V0200cl){
    ClimatData::Pr V0h1cl;
    ClimatData::Pr V0100cl;
    V0100cl.VZ=V0200cl.VZ*0.82;
    V0100cl.VM=V0200cl.VM*0.83;
    V0h1cl.VZ=V0100cl.VZ+(V0200cl.VZ-V0100cl.VZ)*(h1-Constants::level100)/Constants::level100;
    V0h1cl.VM=V0100cl.VM+(V0200cl.VM-V0100cl.VM)*(h1-Constants::level100)/Constants::level100;
    return V0h1cl;
}




