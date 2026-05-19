#ifndef FUNCTIONSCALC_LAYER_H
#define FUNCTIONSCALC_LAYER_H

#include "InData/InData.h"
#include "FunctionsCalc/functionscalc.h"
#include "OutData/outdata.h"
#include "InData/Constants.h"
#include "InData/indataclimat.h"

class FunctionsCalc_Layer
{
public:
    FunctionsCalc_Layer();


    //Получает зональную и меридиальную составляющие скорости в слое 0..hm (ф. 109)
    //если оба соседних уровня ниже hm (вариант 1)
    //i_m - индекс измерения на высоте hm;
    //i_j -
    //VZ_0_hm, VM_0_hm - зональная и меридианальная составляющие скорости в слое 0-hm
    //VZ_m, VM_m - зональная и меридианальная составляющие скорости в на высоте hm
    static bool Get_VZ_VM_Layer_0_hm_0(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int im, int ij, OutData::VLayer &V0hm)
     {
      try{
         double hm= ProfRPV[im].height;
         double hj= ProfRPV[ij].height;
         OutData::V Vj;
         FunctionsCalc::Get_VZ_VM( ProfRPV[im].speed, ProfRPV[im].azimut,V0hm.Vd);
         FunctionsCalc::Get_VZ_VM( ProfRPV[ij].speed, ProfRPV[ij].azimut,Vj);
         //ф.109
            ClimatData::Pr V0hjcl,V0hmcl;
            Climat::Get_Vcl_h(cl,hj,V0hjcl);
            Climat::Get_Vcl_hm(cl,hm,V0hmcl);
            OutData::V delVcl;
            delVcl.VZ=V0hjcl.VZ-V0hmcl.VZ;
            delVcl.VM=V0hjcl.VM-V0hmcl.VM;
            V0hm.Vsr.VZ=(0.5*(V0hm.Vd.VZ+Vj.VZ)*(hj-hm)-hj*delVcl.VZ)/(hj-hm);
            V0hm.Vsr.VM=(0.5*(V0hm.Vd.VM+Vj.VM)*(hj-hm)-hj*delVcl.VM)/(hj-hm);
            return true;
        }
      catch(...)
        {
            return false;
        }

     }

    //Получает зональную и меридиальную составляющие скорости в слое 0..hm (ф. 106)
    //если оба соседних уровня ниже hm (вариант 1)
    //i_m - индекс измерения на высоте hm;
    //i_m_1, i_m_2 - обозначения индексов в соответствии с формулой 106
    //VZ_0_hm, VM_0_hm - зональная и меридианальная составляющие скорости в слое 0-hm
    //VZ_m, VM_m - зональная и меридианальная составляющие скорости в на высоте hm
    //jl=-1 - кроме уровня hm существует всего 1 уровень hj<200м. Тогда hl=100м.
    static bool Get_VZ_VM_Layer_0_hm_1v(PlowAlgoritm::ProfilePoint* ProfRPV, double hm, double hj, double hl, int i_m, int i_j, int i_l, OutData::V V100d,OutData::VLayer &V0hm){
      try{
          int i_m_1=0;
          int i_m_2=0;
          if (hj<hl){
            i_m_2=i_j;
            i_m_1=i_l;
           }
           else {
             i_m_2=i_l;
             i_m_1=i_j;
           }
          double V_m= ProfRPV[i_m].speed;
          double TETA_m= ProfRPV[i_m].azimut;
          FunctionsCalc::Get_VZ_VM(V_m,TETA_m,V0hm.Vd);

          OutData::V Vm1,Vm2;
          double h_m_1,V_m_1,TETA_m_1;
          if (i_m_1>=0){
           h_m_1= ProfRPV[i_m_1].height;
           V_m_1= ProfRPV[i_m_1].speed;
           TETA_m_1= ProfRPV[i_m_1].azimut;
           FunctionsCalc::Get_VZ_VM(V_m_1,TETA_m_1,Vm1);
          }
         else{
            h_m_1=100;
            Vm1=V100d;
          }
          double h_m_2,V_m_2,TETA_m_2;
          if (i_m_2>=0){
           h_m_2= ProfRPV[i_m_2].height;
           V_m_2= ProfRPV[i_m_2].speed;
           TETA_m_2= ProfRPV[i_m_2].azimut;
           FunctionsCalc::Get_VZ_VM(V_m_2,TETA_m_2,Vm2);
          }
          else{
              h_m_2=100;
              Vm1=Vm2;
          }
         V0hm.Vsr.VZ=((Vm1.VZ+V0hm.Vd.VZ)*(hm-h_m_2)+(Vm2.VZ-V0hm.Vd.VZ)*h_m_1)/(2*(hm-h_m_2));
         V0hm.Vsr.VM=((Vm1.VM+V0hm.Vd.VM)*(hm-h_m_2)+(Vm2.VM-V0hm.Vd.VM)*h_m_1)/(2*(hm-h_m_2));
         return true;
        }
      catch(...)
        {
            return false;
        }

     }

    //Получает зональную и меридиальную составляющие скорости в слое 0..hm (ф. 107)
    //если оба соседних уровня выше hm (вариант 2)
    //i_m - индекс измерения на высоте hm;
    //i_m_pl_1, i_m_pl_2 - обозначения индексов в соответствии с формулой 107
    //VZ_0_hm, VM_0_hm - зональная и меридианальная составляющие скорости в слое 0-hm
    //VZ_m, VM_m - зональная и меридианальная составляющие скорости в на высоте hm
    //jl=-1 - кроме уровня hm существует всего 1 уровень hj<200м. Тогда hl=100м.
    static bool Get_VZ_VM_Layer_0_hm_2v(PlowAlgoritm::ProfilePoint* ProfRPV, double hm, double hj, double hl, int i_m, int i_j, int i_l, OutData::V V100d,OutData::VLayer &V0hm){
      try{
            int i_m_pl_1=0;
            int i_m_pl_2=0;
            if (hj<hl){
                i_m_pl_1=i_j;
                i_m_pl_2=i_l;
            }
            else {
                i_m_pl_1=i_l;
                i_m_pl_2=i_j;
            }
            double V_m= ProfRPV[i_m].speed;
            double TETA_m= ProfRPV[i_m].azimut;
            FunctionsCalc::Get_VZ_VM(V_m,TETA_m,V0hm.Vd);

            OutData::V Vmpl1, Vmpl2;
            double h_m_pl_1,V_m_pl_1,TETA_m_pl_1;
            if (i_m_pl_1>=0){
               h_m_pl_1= ProfRPV[i_m_pl_1].height;
               V_m_pl_1= ProfRPV[i_m_pl_1].speed;
               TETA_m_pl_1= ProfRPV[i_m_pl_1].azimut;
               FunctionsCalc::Get_VZ_VM(V_m_pl_1,TETA_m_pl_1,Vmpl1);
            }
            else{
              h_m_pl_1=100;
              Vmpl1=V100d;
            }
            double h_m_pl_2,V_m_pl_2,TETA_m_pl_2;
            if (i_m_pl_2>=0){
               h_m_pl_2= ProfRPV[i_m_pl_2].height;
               V_m_pl_2= ProfRPV[i_m_pl_2].speed;
               TETA_m_pl_2= ProfRPV[i_m_pl_2].azimut;
               FunctionsCalc::Get_VZ_VM(V_m_pl_2,TETA_m_pl_2,Vmpl2);
            }
            else{
                h_m_pl_1=100;
                Vmpl1=V100d;
            }

         V0hm.Vsr.VZ=((V0hm.Vd.VZ+Vmpl1.VZ)*(h_m_pl_2-hm)+(V0hm.Vd.VZ-Vmpl2.VZ)*h_m_pl_1)/(2*(h_m_pl_2-hm));
         V0hm.Vsr.VM=((V0hm.Vd.VM+Vmpl1.VM)*(h_m_pl_2-hm)+(V0hm.Vd.VM-Vmpl2.VM)*h_m_pl_1)/(2*(h_m_pl_2-hm));
         return true;
        }
      catch(...)
        {
            return false;
        }

     }

    //Получает зональную и меридиальную составляющие скорости в слое 0..hm (ф. 108)
    //если один из уровней ниже hm, а другой уровень выше hm (вариант 3)
    //i_m - индекс измерения на высоте hm;
    //i_m_1, i_m_pl_1 - обозначения индексов в соответствии с формулой 108
    //VZ_0_hm, VM_0_hm - зональная и меридианальная составляющие скорости в слое 0-hm
    //VZ_m, VM_m - зональная и меридианальная составляющие скорости в на высоте hm
    //jl=-1 - кроме уровня hm существует всего 1 уровень hj<200м. Тогда hl=100м.
    static bool Get_VZ_VM_Layer_0_hm_3v(PlowAlgoritm::ProfilePoint* ProfRPV, double hj, double hl, int i_m, int i_j, int i_l, OutData::V V100d,OutData::VLayer &V0hm){
      try{
            int i_m_1=0;
            int i_m_pl_1=0;
            if (hj<hl){
                i_m_1=i_j;
                i_m_pl_1=i_l;
            }
            else {
                i_m_1=i_l;
                i_m_pl_1=i_j;
            }

         double V_m= ProfRPV[i_m].speed;
         double TETA_m= ProfRPV[i_m].azimut;
         FunctionsCalc::Get_VZ_VM(V_m,TETA_m,V0hm.Vd);

         OutData::V Vm1, Vmpl1;
         double h_m_1,V_m_1,TETA_m_1;
         if (i_m_1>=0){
             h_m_1= ProfRPV[i_m_1].height;
             V_m_1= ProfRPV[i_m_1].speed;
             TETA_m_1= ProfRPV[i_m_1].azimut;
             FunctionsCalc::Get_VZ_VM(V_m_1,TETA_m_1,Vm1);
        }
         else{
             h_m_1=100;
             Vm1=V100d;
         }
         double h_m_pl_1,V_m_pl_1,TETA_m_pl_1;
         if (i_m_pl_1>=0){
           h_m_pl_1= ProfRPV[i_m_pl_1].height;
           V_m_pl_1= ProfRPV[i_m_pl_1].speed;
           TETA_m_pl_1= ProfRPV[i_m_pl_1].azimut;
          FunctionsCalc::Get_VZ_VM(V_m_pl_1,TETA_m_pl_1,Vmpl1);
         }
         else{//
             h_m_pl_1=100;
             Vmpl1=V100d;
         }
         V0hm.Vsr.VZ=((Vm1.VZ+V0hm.Vd.VZ)*h_m_pl_1-(V0hm.Vd.VZ+Vmpl1.VZ)*h_m_1)/(2*(h_m_pl_1-h_m_1));
         V0hm.Vsr.VM=((Vm1.VM+V0hm.Vd.VM)*h_m_pl_1-(V0hm.Vd.VM+Vmpl1.VM)*h_m_1)/(2*(h_m_pl_1-h_m_1));
         return true;
        }
      catch(...){
            return false;
        }
     }


};

#endif // FUNCTIONSCALC_LAYER_H
