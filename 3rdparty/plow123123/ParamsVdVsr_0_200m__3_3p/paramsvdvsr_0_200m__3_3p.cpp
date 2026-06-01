#include "paramsvdvsr_0_200m__3_3p.h"
#include "FunctionsCalc/functionscalc.h"
#include "InData/Constants.h"
#include "stdlib.h"
#include <iostream>
#include "InData/indataclimat.h"
#include "ParamsVdVsr_0_200m__3_1p/paramsvdvsr_0_200m__3_1p.h"
#include "ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.h"

ParamsVdVsr_0_200m__3_1p _paramsVdVsr_0_200m__3_1p;
ParamsVdVsr_0_200m__3_2p paramsVdVsr_0_200m__3_2p1;

ParamsVdVsr_0_200m__3_3p::ParamsVdVsr_0_200m__3_3p()
{
}

bool ParamsVdVsr_0_200m__3_3p::Do_3_3(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,
                                      OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050,
                                      OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150,OutData::V **VCast){
    try{
       // OutData::VLayer V0200,  V0100, V050;
        //Рассчитываем средний ветер в слое 0-200м
        Get_V_0_200(ProfRPV,cl, V0200,  V0100, V050,V025, V075, V0150,VCast);


        return true;
    }
    catch(...){
        return false;
    }
}

bool ParamsVdVsr_0_200m__3_3p::Get_V_0_200(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl,
                                           OutData::VLayer &V0200, OutData::VLayer &V0100, OutData::VLayer &V050,
                                           OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150,OutData::V **VCast){
    try{
        int i, ipl1;
        //Если интерполяция возможна
        if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level200,i,ipl1)&&
                FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,i,ipl1))
             GetV0200_1s(ProfRPV,cl,i,ipl1,V0200, V0100, V050,V025, V075, V0150, VCast);
        else
             GetV0200_2s(ProfRPV,cl,V0200, V0100, V050,V025, V075, V0150, VCast);

        return true;
    }
    catch(...){
        return false;
    }
}

//Если интерполяция возможна
bool ParamsVdVsr_0_200m__3_3p::Get_V_0_200_1v(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, int i, int ipl1,  OutData::V V0,
                                              OutData::VLayer &V0200_1,int &M,Calc_error::Eps &Eps1){
    try{
        //85
        FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,i,ipl1,Constants::level200,V0200_1.Vd);
        GetV0layer(ProfRPV,Constants::level200,V0,V0200_1.Vd,M,V0200_1.Vsr);
        //Оценка ошибок расчета
        ClimatData::Sigma Si0200;
        //!si Climat::Get_Si(Constants::level200,Climat::Zone,Si0200);
        Si0200=cl->sigma(Constants::level200);
        double RR=Calc_error::Get_RR(0,Constants::level200,ProfRPV);
        double rr=Calc_error::Get_rr(RR);
        Calc_error::Get_Eps(Si0200,rr,Eps1);
        return true;
    }
    catch(...){
        return false;
    }
}

//1-ый способ расчета,
//если возможна интерполяция на уровне 200м
bool ParamsVdVsr_0_200m__3_3p::GetV0200_1s(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,int i, int ipl1,
                                           OutData::VLayer &V0200, OutData::VLayer &V0100,OutData::VLayer &V050,
                                           OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150,OutData::V **VCast){
    try{
        int M;
        OutData::VLayer V0200_1;
        Calc_error::Eps Eps1;
        Get_V_0_200_1v(ProfRPV,cl,i, ipl1,InData::V0ZM,V0200_1,M,Eps1);
        if (M>1)
            V0200=V0200_1;
        else
            //Если есть данные предыдущих зондирований
            if(InData::prevZond)
                V0200.Vsr=paramsVdVsr_0_200m__3_2p1.GetV0200pZ(V0200_1.Vsr,Eps1,VCast);
            else
                V0200=V0200_1;


//100м//(89)
        OutData::V Vh1;
        FunctionsCalc::Get_VZ_VM( ProfRPV[0].speed,  ProfRPV[0].azimut,Vh1);
        int j,jpl1;
        if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level100,j,jpl1))
             FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level100,V0100.Vd);
        else//90
             FunctionsCalc::GetVTeta_LineInterpol0(ProfRPV,InData::V0ZM,Vh1,Constants::level100,V0100.Vd);
        int M1;//91
        GetV0layer(ProfRPV,Constants::level100,InData::V0ZM,V0100.Vd,M1,V0100.Vsr);
//50м
        int k,kpl1;
        if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level50,k,kpl1))
             FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,k,kpl1,Constants::level50,V050.Vd);
        else//94
             FunctionsCalc::GetVTeta_LineInterpol0(ProfRPV,InData::V0ZM,Vh1,Constants::level50,V050.Vd);
        int M2;//95
        GetV0layer(ProfRPV,Constants::level50,InData::V0ZM,V050.Vd,M2,V050.Vsr);
        FunctionsCalc::GetVTeta_LineInterpol3(InData::V0ZM,V050.Vd,InData::h0,
                         Constants::level50,Constants::level25,V025.Vd);
        //int j,jpl1;
        if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level75,j,jpl1))
             FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level75,V075.Vd);//ф.94б
        else
             FunctionsCalc::GetVTeta_LineInterpol4(V050.Vd,V0100.Vd,V075.Vd);//ф.79б
        if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level150,j,jpl1))
             FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level150,V0150.Vd);//ф.94б
        else
             FunctionsCalc::GetVTeta_LineInterpol4(V0100.Vd,V0200.Vd,V0150.Vd);//ф.79б
        return true;
    }
    catch(...){
        return false;
    }
}

//2-ой способ
bool ParamsVdVsr_0_200m__3_3p::GetV0200_2s(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl,OutData::VLayer &V0200, OutData::VLayer &V0100,
                                           OutData::VLayer &V050, OutData::VLayer &V025, OutData::VLayer &V075, OutData::VLayer &V0150,OutData::V **VCast){
    try{
        Calc_error::Eps Eps1;
//100
        OutData::V Vh1;
        FunctionsCalc::Get_VZ_VM( ProfRPV[0].speed,  ProfRPV[0].azimut,Vh1);
        int i,ipl1;
         if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level100,i,ipl1)&&
              FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,i,ipl1))//89
           FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,i,ipl1,Constants::level100,V0100.Vd);
         else{
              if( ProfRPV[0].height>Constants::level100)//90
              FunctionsCalc::GetVTeta_LineInterpol0(ProfRPV,InData::V0ZM,Vh1,Constants::level100,V0100.Vd);
            if( ProfRPV[0].height<Constants::level100){
                int m;
                FunctionsCalc::Poisk_hm(ProfRPV,Constants::level100,m);
                double V100=FunctionsCalc::IndiffStrat(Constants::level100, ProfRPV[m].speed, ProfRPV[m].height);//96
                double TETA100= ProfRPV[m].azimut;
                FunctionsCalc::Get_VZ_VM(V100,TETA100,V0100.Vd);
            }
         }
         int M;
         GetV0layer(ProfRPV,Constants::level100,InData::V0ZM,V0100.Vd,M,V0100.Vsr);
//50
         int j,jpl1;
         if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level50,j,jpl1)&&
              FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,j,jpl1))//93
           FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level50,V050.Vd);
         else{
             if( ProfRPV[0].height>Constants::level50)//94
               FunctionsCalc::GetVTeta_LineInterpol0(ProfRPV,InData::V0ZM,Vh1,Constants::level50,V050.Vd);
             if( ProfRPV[0].height<Constants::level50){
                 int m1;
                 FunctionsCalc::Poisk_hm(ProfRPV,Constants::level50,m1);
                 double V50=FunctionsCalc::IndiffStrat(Constants::level50, ProfRPV[m1].speed, ProfRPV[m1].height);//97
                 double TETA50= ProfRPV[m1].azimut;
                 FunctionsCalc::Get_VZ_VM(V50,TETA50,V050.Vd);
             }
         }
         GetV0layer(ProfRPV,Constants::level50,InData::V0ZM,V050.Vd,M,V050.Vsr);
//200
         OutData::V V0200_1;
         int m2;
         FunctionsCalc::Poisk_hm(ProfRPV,Constants::level200,m2);
         OutData::V Vhm, V0hm;
         int M2;
         FunctionsCalc::Get_VZ_VM( ProfRPV[m2].speed, ProfRPV[m2].azimut,Vhm);
         GetV0layer(ProfRPV, ProfRPV[m2].height,InData::V0ZM,Vhm,M2,V0hm);//98
         //Существует ли уровень измерения локатора hm+1 c полученными для него значениями V
         if (((m2+1)<InData::k)&&(ProfRPV[m2+1].curr==1)){
           GetV0200_1v_mpl1(ProfRPV,cl, m2,Vhm,V0hm,V0200_1,Eps1);
         }
         else{
             V0200_1.VZ=V0100.Vsr.VZ/0.82;
             V0200_1.VM=V0100.Vsr.VM/0.83;
             ClimatData::Sigma Si0200;
             Si0200=cl->sigma(Constants::level200);
             Eps1.EpsZ=Si0200.SiVZ;
             Eps1.EpsM=Si0200.SiVM;
         }
         //В случае наличия предыдущих зондирований
         OutData::V V0200_2;
         Calc_error::Eps Eps2;
         if (InData::prevZond)
             V0200.Vsr=paramsVdVsr_0_200m__3_2p1.GetV0200pZ(V0200_1,Eps1,VCast);
         else
             V0200.Vsr=V0200_1;

         double h0=InData::h0;
         double hm= ProfRPV[m2].height;
         V0200.Vd.VZ=2*(V0200.Vsr.VZ*(Constants::level200-h0)-V0hm.VZ*(hm-h0))/(Constants::level200-hm)-Vhm.VZ;
         V0200.Vd.VM=2*(V0200.Vsr.VM*(Constants::level200-h0)-V0hm.VM*(hm-h0))/(Constants::level200-hm)-Vhm.VM;

         FunctionsCalc::GetVTeta_LineInterpol3(InData::V0ZM,V050.Vd,InData::h0,
                          Constants::level50,Constants::level25,V025.Vd);
         //int j,jpl1;
         if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level75,j,jpl1))
              FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level75,V075.Vd);//ф.94б
         else
              FunctionsCalc::GetVTeta_LineInterpol4(V050.Vd,V0100.Vd,V075.Vd);//ф.79б
         if (FunctionsCalc::PoiskBoundsLevels(ProfRPV,Constants::level150,j,jpl1))
              FunctionsCalc::GetVTeta_LineInterpol2(ProfRPV,j,jpl1,Constants::level150,V0150.Vd);//ф.94б
         else
              FunctionsCalc::GetVTeta_LineInterpol4(V0100.Vd,V0200.Vd,V0150.Vd);//ф.79б
         return true;
     }
     catch(...){
         return false;
     }
 }


bool ParamsVdVsr_0_200m__3_3p::GetV0200_1v_mpl1(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int m, OutData::V Vhm,
                                                OutData::V V0hm,OutData::V &V0200_1, Calc_error::Eps &Eps1){
  try{
    V0200_1=GetV0200_f100(ProfRPV, m, Vhm, V0hm);
    double Yl=Constants::level200;//???
    double hm=ProfRPV[m].height;
    double hmpl1=ProfRPV[m+1].height;
    Eps1=Calc_error::Get_Eps_f102( cl, Yl, hm, hmpl1);//ф.102
    return true;
    }
    catch(...){
        return false;
    }
}

OutData::V ParamsVdVsr_0_200m__3_3p::GetV0200_f100(PlowAlgoritm::ProfilePoint* ProfRPV, int m, OutData::V Vhm,
                         OutData::V V0hm){
    OutData::V V0200_1;
    OutData::V Vhmpl1,delV;
    FunctionsCalc::Get_VZ_VM(ProfRPV[m+1].speed,ProfRPV[m+1].azimut,Vhmpl1);
    double hm=ProfRPV[m].height;
    double hmpl1=ProfRPV[m+1].height;
    delV.VZ=(Vhmpl1.VZ-Vhm.VZ)*pow((Constants::level200-hm),2)/(2*(Constants::level200-InData::h0)*(hmpl1-hm))+//ф.101
            (Constants::level200-hm)*(Vhm.VZ-V0hm.VZ)/(Constants::level200-InData::h0);
    delV.VM=(Vhmpl1.VM-Vhm.VM)*pow((Constants::level200-hm),2)/(2*(Constants::level200-InData::h0)*(hmpl1-hm))+
            (Constants::level200-hm)*(Vhm.VM-V0hm.VM)/(Constants::level200-InData::h0);
    V0200_1.VZ=V0hm.VZ+delV.VZ;//ф.100
    V0200_1.VM=V0hm.VM+delV.VM;
    return V0200_1;
}

OutData::V ParamsVdVsr_0_200m__3_3p::GetV0200_f103(const ClimatData* cl, double hmpl1, OutData::V V0hmpl1){
    OutData::V V0200_1;
    ClimatData::Pr Vcl0hmpl1,Vcl0200;
    Climat::Get_Vcl_h(cl,hmpl1,Vcl0hmpl1);
    Climat::Get_Vcl_h(cl,Constants::level200,Vcl0200);
    V0200_1.VZ=V0hmpl1.VZ-Vcl0hmpl1.VZ+Vcl0200.VZ;//103
    V0200_1.VM=V0hmpl1.VM-Vcl0hmpl1.VM+Vcl0200.VM;
    return V0200_1;
}

OutData::V ParamsVdVsr_0_200m__3_3p::GetV0200_f101(double hm, double hmpl1, OutData::V V0hm,OutData::V V0hmpl1){
    OutData::V V0200_1;
    V0200_1.VZ=V0hm.VZ*(hmpl1-Constants::level200)/(hmpl1-hm)+V0hmpl1.VZ*(Constants::level200-hm)/(hmpl1-hm);//101
    V0200_1.VM=V0hm.VM*(hmpl1-Constants::level200)/(hmpl1-hm)+V0hmpl1.VM*(Constants::level200-hm)/(hmpl1-hm);
    return V0200_1;
}


//Оценка параметров среднего ветра в слое 0-layer (ф.86)
//V0 - скорость приземного ветра;
//Vlayer - скорость действительного ветра на высоте layer
//M - количество уровней измерения локатора в пределах
//слоя 0-layer, не считая верхнюю границу
bool ParamsVdVsr_0_200m__3_3p::GetV0layer(PlowAlgoritm::ProfilePoint* ProfRPV, double layer, OutData::V V0, OutData::V Vlayer, int &M, OutData::V &V0layer){
    try{
        int im;//уровень измерений локаротора, ближайшийо снизу от уровня layer
        bool b =FunctionsCalc::Poisk_hm(ProfRPV,layer,im);
        M=im+1;
        int num=im+2;
//        int num=im+1;
        if (num >= 96)
            cout << "!!!!!!!!!!!" << num << endl;
        if (im != InData::k-1) {
            if ( ProfRPV[im+1].height!=layer)
                num++;
            if ( ProfRPV[im+1].height!=0)
                num++;
        }

        L Lmas[num];
        Lmas[0].hi=InData::h0;
        Lmas[0].Vi.VZ=V0.VZ;
        Lmas[0].Vi.VM=V0.VM;
        Lmas[num-1].hi=layer;
        Lmas[num-1].Vi.VZ=Vlayer.VZ;
        Lmas[num-1].Vi.VM=Vlayer.VM;
        for (int i=1;i<(num-1);i++){
            FunctionsCalc::Get_VZ_VM( ProfRPV[i-1].speed, ProfRPV[i-1].azimut,Lmas[i].Vi);
            Lmas[i].hi= ProfRPV[i-1].height;
        }
        double sumZ=0, sumM=0;
        for (int l=0;l<(num-1);l++){//???????!!!!!!!!!!!!!!!!!
            sumZ+=(Lmas[l].Vi.VZ+Lmas[l+1].Vi.VZ)*(Lmas[l+1].hi-Lmas[l].hi);
            sumM+=(Lmas[l].Vi.VM+Lmas[l+1].Vi.VM)*(Lmas[l+1].hi-Lmas[l].hi);
        }
        V0layer.VZ=0.5*sumZ/(layer-InData::h0);
        V0layer.VM=0.5*sumM/(layer-InData::h0);

        for (int l=0;l<num;l++)
       // std::cout <<Lmas[l].Vi.VZ<<"\t"<<Lmas[l].Vi.VM<<"\t"<<Lmas[l].hi<<"\r\n" ;
        return true;
    }
    catch(...){
       return false;
    }
    return false;

}






