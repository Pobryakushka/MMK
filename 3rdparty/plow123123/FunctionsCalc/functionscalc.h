#ifndef FUNCTIONSCALC_H
#define FUNCTIONSCALC_H

#define _USE_MATH_DEFINES

#include "InData/InData.h"
#include "FunctionsCalc/mathfunc.h"
#include "math.h"
#include <math.h>
#include "OutData/outdata.h"
#include "InData/indataclimat.h"
#include "mhn/structures.h"

class FunctionsCalc
{
public:
//Признак
static int INT100;//?????

//Невязки
struct delV{
   double Z;
   double M;
   delV(){Z=0; M=0;}
};

//Коэффициенты
struct K{
   double Z;
   double M;
   K(){Z=0;M=0;}
};

//Градиент
struct Gr{
   double Z;
   double M;
   Gr(){Z=0;M=0;}
};

public:
    FunctionsCalc();


    //Поиск двух соседних уровней, между которыми расположен
    //расчетный уровень h, м (83)
    //true - уровни найдены, false - не найдены.
    static bool PoiskBoundsLevels(PlowAlgoritm::ProfilePoint* ProfRPV, double h, int &ind_i, int &ind_i_pl_1)
        {
           ind_i=0;
           ind_i_pl_1=0;
           if ( ProfRPV[0].height>=h)
           {
               return false;
           }
            for (int i=0;i<InData::k;i++)
            {
              if ( ProfRPV[i].height>=h)
                  if ( ProfRPV[i-1].height<=h)
                      {
                          ind_i=i-1;
                          ind_i_pl_1=i;
                          return true;
                      }
            }
             return false;
        }

    //Поиск двух соседних уровней в списке стандарьных слоев (для МБ),
    //между которыми расположен расчетный уровень h, м (83)
    //true - уровни найдены, false - не найдены.
    static bool PoiskBoundsLevelsMB(double h, int &ind_i, int &ind_i_pl_1){
        int isMB=Climat::Get_is_MB(h);
        //Если h - стандартный уровень МБ
        if(isMB>0){
             ind_i=isMB;
             ind_i_pl_1=isMB;
             return true;
          }
           ind_i=0;
           ind_i_pl_1=0;
//           if (Climat::StandartLayer[0]>=h)
//           {
//               return false;
//           }
            for (int i=0;i<Constants::numStL_MB;i++){
              if (Constants::StandartLayerMB[i]>=h)
                  if (Constants::StandartLayerMB[i-1]<=h)
                      {
                          ind_i=i-1;
                          ind_i_pl_1=i;
                          return true;
                      }
            }
            return false;
        }

   //Проверка найденных 2-х соседних уровней (84)
    static bool ProverkaBoundsLevels(PlowAlgoritm::ProfilePoint* ProfRPV, double h, int ind_i, int ind_i_pl_1){
       if (abs( ProfRPV[ind_i_pl_1].height- ProfRPV[ind_i].height)<=h)
           return true;
       return false;
    }

    //Получает индекс высоты h в массиве ProfRPV
    static int Get_indh(PlowAlgoritm::ProfilePoint* ProfRPV, double h){
        int ih=-1;
        try{
          for (int i=(InData::k-1);i>=0;i--){
              if ( ProfRPV[i].height==h){
                  ih=i;
                  return ih;
                 }
          }
          return ih;
        }
        catch(...){
            return ih;
        }
    }

    //Поиск уровня измерений локаротора, ближайшего снизу от уровня h
    static bool Poisk_hm(PlowAlgoritm::ProfilePoint* ProfRPV, double h,int &i_m){
        i_m=-1;
        try{
          for (int i=(InData::k-1);i>=0;i--){
              if ( ProfRPV[i].height<=h){
                  i_m=i;
                  return true;
                 }
          }
          return false;
        }
        catch(...){
            return false;
        }
    }

   //Поиск уровня измерений локаротора, ближайшего сверху от уровня h
   static bool Poisk_hm_ontop(PlowAlgoritm::ProfilePoint* ProfRPV, double h,int &i_m){
       i_m=-1;
       try{
         for (int i=0;i<InData::k;i++){
            if ( ProfRPV[i].height>h){
               /*????не уверена!!! if (i>0)*/{
                 i_m=i;
                 return true;
                }
              //????не уверена!!!!  return false;
             }
         }
         return false;
       }
       catch(...){
           return false;
       }
   }

   //Поиск уровня из массива mas, ближайшего сверху от уровня h или равного h
   static bool Poisk_hm_ontop_mas(double* mas, int n, double h,int &i_m){
       i_m=-1;
       try{
         for (int i=0;i<n;i++){
            if ( mas[i]>=h){
                if (i>0){
                 i_m=i;
                 return true;
                }
                return false;
             }
         }
         return false;
       }
       catch(...){
           return false;
       }
   }

   //Поиск двух уровней измерений локатора, ближайших по отношению к hm
   static bool PoiskNearestLevels_hm(PlowAlgoritm::ProfilePoint* ProfRPV, int i_m, int &i_j, int &i_l)
   {
       double mas_del[]={-1,-1,-1,-1};
       int n=i_m-1;
       int k=0;
       int j=0;
       int l=0;
       i_j=0;
       i_l=0;
       try
       {
         while (n>=0)
         {
             if (k>=2)
                 break;
             mas_del[k]=abs( ProfRPV[i_m].height- ProfRPV[n].height);
             n--;
             k++;
         }
         n=i_m+1;
         k=3;
         while (n<InData::k)
         {
             if (k==1)
                 break;
             mas_del[k]=abs( ProfRPV[i_m].height- ProfRPV[n].height);
             n++;
             k--;
         }
         MathFunc::Poisk_min_abs(mas_del,4,j);
         mas_del[j]=-1;
         MathFunc::Poisk_min_abs(mas_del,4,l);
          mas_del[l]=-1;
         Equiv_index_im(i_m, j, i_j);
         Equiv_index_im(i_m, l, i_l);
         return true;
       }
       catch(...)
       {
             return false;
       }
   }

  //Вспомогательная Функция приведения индексов (для PoiskNearestLevels_hm)
   static void Equiv_index_im(int i_m, int ind, int &i_i)
   {
      i_i=0;
       switch (ind) {
       case 0:
           i_i=i_m-1;
           break;
       case 1:
           i_i=i_m-2;
           break;
       case 2:
           i_i=i_m+2;
           break;
       case 3:
           i_i=i_m+1;
           break;
       }
   }

   //Поиск индекса уровня, ближайшего к высоте h
   static int Poisk_imsh(PlowAlgoritm::ProfilePoint* ProfRPV, double h){
       int imax=-1,imin=-1;
       for (int i=0;i<InData::k;i++)
           if ( ProfRPV[i].height>h){
               imax=i;
               if (i>0)
                imin=i-1;
               else imin=imax;
               break;
           }
         if((imax>=0) && (imax>=0)){
          double del1,del2;
          del1=abs(h- ProfRPV[imin].height);
          del2=abs(h- ProfRPV[imax].height);
          if (del1<del2) return imin;
           else return imax;
         }
         else return -1;
   }

   //Поиск индекса уровня, ближайшего к высоте h снизу
   static int Poisk_imsh_low(double h){
       int i_m=-1;
       try{
         for (int i=/*Constants::numStL*/Climat::numStL-1;i>=0;i--){
             if (Climat::StandartLayer[i]<=h){
                 i_m=i;
                 return i_m;
                }
         }
         return i_m;
       }
       catch(...){
           return i_m;
       }
   }

   //Расчет скорсти и направления ветра
   //по формулам безразличной стратификации (96, 97)
   //Vh(м), TETAh(градусы) - скорость и направление действительного ветра на уровне h
   static bool GetVTeta_IndiffStrat(PlowAlgoritm::ProfilePoint* ProfRPV, double h, double &Vh, double &TETAh)
   {
       Vh=0;
       TETAh=0;
       double Vhi=0;
       double TETAhi=0;
       double hi=0;
       int i_hi=0;
       try
       {
           //Поиск ближайшего уроня к h
           for (int i=0;i<InData::k;i++)
           {
             if ( ProfRPV[i].height==h)
                 INT100=1;
             if ( ProfRPV[i].height>=h)
             {
                 if (( ProfRPV[i].height-h)<(h- ProfRPV[i-1].height))
                    i_hi=i;
                 else i_hi=i-1;
               break;
             }
           }

          Vhi= ProfRPV[i_hi].speed;
          TETAhi= ProfRPV[i_hi].azimut;
          hi= ProfRPV[i_hi].height;
          Vh=Vhi*(log((h+InData::z0)/InData::z0)/log((hi+InData::z0)/InData::z0));
          TETAh=TETAhi;
          return true;
       }
       catch(...)
       {
           return false;
       }
   }



   //Логарифмическая формула для случая безразличной стратификации
   //ф.63
   static double IndiffStrat(double h, double V0,double h0){
       double Vh=0;
       try{
           Vh=(double)V0*(log((h+InData::z0)/InData::z0)/log((h0+InData::z0)/InData::z0));
           return  Vh;
       }
       catch(...){
           return  Vh;
       }
   }

   //Получает разницы(невязки) между скоростями V1 и V2
   static delV GetdelV(OutData::V V1, OutData::V V2){
       delV _delV;
       _delV.Z=fabs(V1.VZ-V2.VZ);
       _delV.M=fabs(V1.VM-V2.VM);
       return _delV;
   }

   //Расчет скорсти и направления ветра
   //по формулам линейной интерполяции (85)
   static bool GetVTeta_LineInterpol(PlowAlgoritm::ProfilePoint* ProfRPV, int ind_i, int ind_i_pl_1, double h, double &VZ, double &VM)
   {
     VZ=0;
     VM=0;
     double VZ_i=0;
     double VM_i=0;
     double VZ_i1=0;
     double VM_i1=0;
     double h_i= ProfRPV[ind_i].height;
     double h_i1= ProfRPV[ind_i_pl_1].height;
       try
       {
         Get_VZ_VM( ProfRPV[ind_i].speed,  ProfRPV[ind_i].azimut,  VZ_i, VM_i);
         Get_VZ_VM( ProfRPV[ind_i_pl_1].speed,  ProfRPV[ind_i_pl_1].azimut,  VZ_i1, VM_i1);
         VZ=VZ_i+(VZ_i1-VZ_i)*(h-h_i)/(h_i1-h_i);
         VM=VM_i+(VM_i1-VM_i)*(h-h_i)/(h_i1-h_i);
         return true;
       }
     catch(...)
       {
         return false;
       }
   }

   //Расчет скорсти и направления ветра
   //по формулам линейной интерполяции (85)
   static bool GetVTeta_LineInterpol2(PlowAlgoritm::ProfilePoint* ProfRPV, int ind_i, int ind_i_pl_1, double h, OutData::V &VINT)
   {
     OutData::V Vi,Vi1;
     double h_i= ProfRPV[ind_i].height;
     double h_i1= ProfRPV[ind_i_pl_1].height;
       try
       {
         Get_VZ_VM( ProfRPV[ind_i].speed,  ProfRPV[ind_i].azimut,  Vi);
         Get_VZ_VM( ProfRPV[ind_i_pl_1].speed,  ProfRPV[ind_i_pl_1].azimut, Vi1);
         VINT.VZ=Vi.VZ+(Vi1.VZ-Vi.VZ)*(h-h_i)/(h_i1-h_i);
         VINT.VM=Vi.VM+(Vi1.VM-Vi.VM)*(h-h_i)/(h_i1-h_i);
         return true;
       }
     catch(...)
       {
         return false;
       }
   }

   //Расчет скорсти и направления ветра
   //по формулам линейной интерполяции (85)для МБ(метеобюллетеня)
   //numMB - номер МБ
   static bool GetVTeta_LineInterpol2_MB(int numMB, int ind_i, int ind_i_pl_1, double h,OutData::V** VCast, OutData::V &VINT)
   {
       //если интерполяция не требуется
       if(ind_i==ind_i_pl_1){
         VINT=VCast[numMB-1][ind_i];
         return true;
       }
     OutData::V Vi=VCast[numMB-1][ind_i];
     OutData::V Vi1=VCast[numMB-1][ind_i_pl_1];
     double h_i=InData::MBulletin_table[numMB-1].ProfMeteo[ind_i].h;
     double h_i1=InData::MBulletin_table[numMB-1].ProfMeteo[ind_i_pl_1 ].h;
       try
       {
         VINT.VZ=Vi.VZ+(Vi1.VZ-Vi.VZ)*(h-h_i)/(h_i1-h_i);
         VINT.VM=Vi.VM+(Vi1.VM-Vi.VM)*(h-h_i)/(h_i1-h_i);
         return true;
       }
     catch(...)
       {
         return false;
       }
   }

   //Расчет скорсти и направления ветра
   //по формулам линейной интерполяции (89)
   //когда 1-ый уровень измерения локатора >h
   static bool GetVTeta_LineInterpol0(PlowAlgoritm::ProfilePoint* ProfRPV, OutData::V V0,OutData::V V1,double h, OutData::V &Vh){
       try{
          double h1= ProfRPV[0].height;
          double h0=InData::h0;
           Vh.VZ=V0.VZ+(V1.VZ-V0.VZ)*(h-h0)/(h1-h0);
           Vh.VM=V0.VM+(V1.VM-V0.VM)*(h-h0)/(h1-h0);
          return true;
       }
     catch(...){
         return false;
       }
   }

   //Расчет действительного ветра
   //по формулам линейной интерполяции (79а)
   //h2>h1,
   //hd - уровень, на котором рассчитывается действ. ветер
   static bool GetVTeta_LineInterpol3(OutData::V V1,OutData::V V2,double h1, double h2, double hd, OutData::V &Vhd){
       try{

           Vhd.VZ=V1.VZ+(V2.VZ-V1.VZ)*(hd-h1)/(h2-h1);
           Vhd.VM=V1.VM+(V2.VM-V1.VM)*(hd-h1)/(h2-h1);
          return true;
       }
     catch(...){
         return false;
       }
   }

   //Расчет действительного ветра
   //по формулам линейной интерполяции (79б, 79в)
   static bool GetVTeta_LineInterpol4(OutData::V V1,OutData::V V2,OutData::V &Vhd){
       try{

           Vhd.VZ=(V1.VZ+V2.VZ)/2;
           Vhd.VM=(V1.VM+V2.VM)/2;
          return true;
       }
     catch(...){
         return false;
       }
   }

   //Получает зональную и меридиальную составляющие (2, 38)
   //V (м), TETA (градусы)
   static bool Get_VZ_VM(double V, double TETA, double &VZ, double &VM)
   {
       VZ=0;
       VM=0;
       try
       {

           VZ=-V*sin(TETA*M_PI/180);
           VM=-V*cos(TETA*M_PI/180);
           return true;
       }
       catch(...)
       {
           return false;
       }
   }

   //Получает зональную и меридиальную составляющие (2, 38)
   //V (м), TETA (градусы)
   static bool Get_VZ_VM(double V, double TETA, OutData::V &VZM)
   {
       VZM.VZ=0; VZM.VM=0;
       try{
           VZM.VZ=-V*sin(TETA*M_PI/180);
           VZM.VM=-V*cos(TETA*M_PI/180);
           return true;
       }
       catch(...)
       {
           return false;
       }
   }

   //ф.25
   static bool Get_VZ_VM_inv(OutData::V VZM, double &V, double &TETA)
   {
       try{
           V=pow(VZM.VZ,2)+pow(VZM.VM,2);
           V=pow(V,0.5);
           if(VZM.VM<=0)
             TETA=asin(-VZM.VZ/V)*180/M_PI;
           else TETA=180-(asin(-VZM.VZ/V)*180/M_PI);
           if (TETA<0) TETA+=360;

           return true;
       }
       catch(...)
       {
           return false;
       }
   }

   //ф.38а
   //Получает градиент
   static Gr GetGr(OutData::V Vi, OutData::V Vn1, double hi, double hn1){
       Gr gri;
       gri.Z=abs((Vi.VZ-Vn1.VZ)/(hi-hn1));
       gri.M=abs((Vi.VM-Vn1.VM)/(hi-hn1));
       return gri;
   }


   //Получает знаковую разницу между двумя вещественными величинами
   /*static double GetDiff(double d1, double d2){
       return d2-d1;
   }*/

   //----------------------
/*   static void Getmas(int i, double *mas1,double *mas2){

       mas1[0]=1.1;
       mas1[1]=2.2;
       mas2[0]=3.3;
       mas2[1]=4.4;

   }*/


  static void printArray(double (*arr)[3], unsigned size1) {
       unsigned i, j;
       for (i = 0; i < size1; i++) {
           for (j = 0; j < 3; j++) {
                arr[i][j]=i+j;
           }
       }
   }

  static void Arr2(float **p, int r, int c){
      for (int i = 0; i < r; i++) {
          for (int j = 0; j < c; j++) {
               p[i][j]=i+j;
          }
      }
  }

};



#endif // FUNCTIONSCALC_H
