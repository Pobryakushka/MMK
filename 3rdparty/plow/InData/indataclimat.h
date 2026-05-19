#ifndef CLIMAT_H
#define CLIMAT_H

//#include "math.h";
#include "OutData/outdata.h"
#include "InData/Constants.h"
#include "InData.h"
//#include "Profile/profile.h"
#include "climatdata.h"

//#include <iostream>

class Climat
{
public:
    static int Zone;

    //список стандартных слоев
  //  static const int numStL/*=12*/;
    static const int numStL=15;//Кол-во стандартных слоев климатической БД
    static double  StandartLayer[] ;

    //Значения среднеквадратических отклонений зональной и меридиональной составляющих
    //среднего ветра для стандартного слоя.
    struct Sigma{
       double SiVZ;
       double SiVM;
   };
    //static Sigma ClimatSi[полугодие][зона][слой][месяц];
//    static Sigma ClimatSi[2][11][12][12];

    //Среднеклиматические значения параметров ветра
    //из климатической БД
 /*   struct Pr{
       double VZ;
       double VM;
   };*/
    //static Pr ClimatPr[полугодие][зона][слой][месяц];
//    static /*Pr*/OutData::V ClimatPr[2][11][12][12];

    //Коэффициенты линейной автокорреляции между слоями
    //из климатической БД
    struct Correl{
       double CorrZ;
       double CorrM;
   };
    //static Correl ClimatCorrel[зона][месяц][слой1][слой2];
    static Correl ClimatCorrel[11][12][11][12];
 //   const ClimatData* cl;

 public:
    Climat();
    bool Get_ClimatSi();
    void ReadClimatSi(char *mas[]);
    void ReadClimatPr(char *mas[]);
    void ReadClimatCorrel(char *mas[]);
    bool Get_ClimatPr();
    bool Get_ClimatCorrel();

    //Получает номер климатичсекой зоны     !!!!!!Дописать функцию
    static int Get_Zone(){
      return 4;
    }

   //Получает индекс-признак типа полугодия:
   //0 - холодное полугодие;
   //1 - теплое полугодие.
    static int Get_ip(int MS0){
       if ((MS0<=9)&&(MS0>=4))
           return 1;
       else return 0;
   }

    //Получае индекс стандартного слоя
    static int Get_is(double level){
       int is=-1;
        for (int i=0;i</*15*/numStL;i++){
           if (StandartLayer[i]==level){
              is=i;
              break;
           }
        }
         return is;
    }

    //Получае индекс стандартного слоя,
    //ближайшего сверху к h
    static int Get_is_h(double h){
       int is=-1;
       for (int i=0;i</*15*/numStL;i++){
           if (h<=StandartLayer[i]){
               is=i;
               break;
           }
       }
       return is;
    }

    //Получае индекс стандартного слоя,
    //ближайшего снизу к h
    static int Get_is_h_low(double h){
       int is=-1;
       for (int i=(/*Constants::numStL*//*15*/numStL-1);i>=0;i--){
           if (h>=StandartLayer[i]){
               is=i;
               break;
           }
       }
       return is;
    }

    //Получае индекс стандартного слоя для МБ
    static int Get_is_MB(double level){
       int is=-1;
        for (int i=0;i<Constants::numStL_MB;i++){
           if (Constants::StandartLayerMB[i]==level){
              is=i;
              break;
           }
        }
         return is;
    }

    //Получае индекс стандартного слоя,
    //ближайшего сверху к h (для МБ)
    static int Get_is_h_MB(double h){
       int is=-1;
       for (int i=0;i<Constants::numStL_MB;i++){
           if (h<=Constants::StandartLayerMB[i]){
               is=i;
               break;
           }
       }
       return is;
    }

    //Получает среднеклиматичсекие изменчивости составляющих среднего ветра Si_VZ и Si_VM
    //в слое 0-level_standart м, которые непосредственно определяются из климатической БД
//    static bool Get_Si(double level,int zone/*,int MS0*/,Sigma &Si){
//     int ip=Get_ip(InData::Tau0::MS0);
//     int is=Get_is(level);
//     int iz=zone-1;
//     int im=InData::Tau0::MS0-1;

//      Si.SiVZ=ClimatSi[ip][iz][is][im].SiVZ;//6.4
//      Si.SiVM=ClimatSi[ip][iz][is][im].SiVM;//4
//      return true;
//    }

    //Получает среднеклиматичсекие изменчивости составляющих среднего ветра SiZ_0_h и SiM_0_h
    //в слое 0-h м
    static bool Get_Si_h( const ClimatData* cl,double h,ClimatData::Sigma &Si){
        try{


            int is=Get_is(h);
            //Если h>8000м
            if (h>Climat::StandartLayer[/*11*/14]){
             //!si  Get_Si(Climat::StandartLayer[/*11*/14],Climat::Zone,Si);
                Si=cl->sigma(Climat::StandartLayer[/*11*/14]);
               return true;
            }
            //если h - стандартный слой
            if (is>=0){
               //!si Get_Si(h,Climat::Zone,Si);
                Si=cl->sigma(h);
            }
            else{
                is=Get_is_h(h);
              //!si  Get_Si(StandartLayer[is],Climat::Zone,Si);
                Si=cl->sigma(StandartLayer[is]);
            }
            return true;
        }
        catch(...){
            return false;
        }
    }

//    //Получает среднеклиматичсекие изменчивости составляющих среднего ветра SiZ_0_h и SiM_0_h
//    //в слое 0-h м
//    static bool Get_Si_h(double h,ClimatData::Sigma &Si){
//        try{


//            int is=Get_is(h);
//            //Если h>8000м
//            if (h>Climat::StandartLayer[/*11*/14]){
//             //!si  Get_Si(Climat::StandartLayer[/*11*/14],Climat::Zone,Si);
//                Si=cl->sigma(Climat::StandartLayer[/*11*/14]);
//               return true;
//            }
//            //если h - стандартный слой
//            if (is>=0){
//               //!si Get_Si(h,Climat::Zone,Si);
//                Si=cl->sigma(h);
//            }
//            else{
//                is=Get_is_h(h);
//              //!si  Get_Si(StandartLayer[is],Climat::Zone,Si);
//                Si=cl->sigma(StandartLayer[is]);
//            }
//            return true;
//        }
//        catch(...){
//            return false;
//        }
//    }

//    //Получает среднеклиматические значения параметров ветра
//    //из климатической БД
//    static bool Get_Vcl(double level,int zone,double &VZ,double &VM){
//       int ip=Get_ip(InData::Tau0::MS0);
//       int is=Get_is(level);
//       int iz=zone-1;
//       int im=InData::Tau0::MS0-1;

//        VZ=ClimatPr[ip][iz][is][im].VZ;
//        VM=ClimatPr[ip][iz][is][im].VM;
//        return true;
//   }

//    //Получает среднеклиматические значения параметров ветра
//    //из климатической БД
//    static bool Get_Vcl(double level,int zone,OutData::V &Vcl){
//       int ip=Get_ip(InData::Tau0::MS0);
//       int is=Get_is(level);
//       int iz=zone-1;
//       int im=InData::Tau0::MS0-1;

//       Vcl.VZ=ClimatPr[ip][iz][is][im].VZ;
//       Vcl.VM=ClimatPr[ip][iz][is][im].VM;
//        return true;
//   }

//    //Получает среднеклиматические значения параметров среднего ветра в слое 0-h,
//    //полученные путем линейной интерполяции по данным о среднеклимтических значениях
//    //в стандартных слоях.
//    static bool Get_Vcl_h(double h, double &VZ_0_h, double &VM_0_h){
//        try{
//            int mpl1;
//            int m;
//            double Ympl1;
//            double Ym;
//            double VZm=0, VMm=0;
//            double VZmpl1=0,VMmpl1=0;
//            int is=Get_is(h);
//            //Если h>8000м
//            if (h>Climat::StandartLayer[11]){
//               Get_Vcl(Climat::StandartLayer[11],Climat::Zone,VZ_0_h,VM_0_h);
//               return true;
//            }
//            //если h - стандартный слой
//            if (is>=0){
//                Ym=StandartLayer[is];
//                Ympl1=Ym;
//                Get_Vcl(Ym,Climat::Zone,VZm,VMm);
//                VZ_0_h=VZm;
//                VM_0_h=VMm;
//            }
//            else{
//                mpl1=Get_is_h(h);
//                m=mpl1-1;
//                Ympl1=StandartLayer[mpl1];
//                Ym=StandartLayer[m];
//                Get_Vcl(Ym,Climat::Zone,VZm,VMm);
//                Get_Vcl(Ympl1,Climat::Zone,VZmpl1,VMmpl1);
//                //ф. 44, ф.5, ф.6, ф.7
//                VZ_0_h=VZm+(VZmpl1-VZm)*(h-Ym)/(Ympl1-Ym);
//                VM_0_h=VMm+(VMmpl1-VMm)*(h-Ym)/(Ympl1-Ym);
//            }
//            return true;
//        }
//        catch(...){
//            return false;
//        }
//    }

    //Получает среднеклиматические значения параметров среднего ветра в слое 0-h,
    //полученные путем линейной интерполяции по данным о среднеклимтических значениях
    //в стандартных слоях.
    static bool Get_Vcl_h(const ClimatData* cl, double h, ClimatData::Pr &V0h){
        try{
            int mpl1;
            int m;
            double Ympl1;
            double Ym;
            ClimatData::Pr Vm, Vmpl1;

            int is=Get_is(h);
            //Если h>8000м
            if (h>Climat::StandartLayer[/*11*/14]){
               //!pr Get_Vcl(Climat::StandartLayer[/*11*/14],Climat::Zone,V0h);
                V0h=cl->pr(Climat::StandartLayer[/*11*/14]);
               return true;
            }
            //если h - стандартный слой
            if (is>=0){
                Ym=StandartLayer[is];
                Ympl1=Ym;
                //!pr Get_Vcl(Ym,Climat::Zone,Vm);
                Vm=cl->pr(Ym);
                V0h=Vm;
            }
            else{
                mpl1=Get_is_h(h);
                m=mpl1-1;
                Ympl1=StandartLayer[mpl1];
                Ym=StandartLayer[m];
                //!pr Get_Vcl(Ym,Climat::Zone,Vm);
                Vm=cl->pr(Ym,-1);
                //!pr Get_Vcl(Ympl1,Climat::Zone,Vmpl1);
                Vmpl1=cl->pr(Ympl1,-1);
                //ф. 44, ф.5, ф.6, ф.7
                V0h.VZ=Vm.VZ+(Vmpl1.VZ-Vm.VZ)*(h-Ym)/(Ympl1-Ym);
                V0h.VM=Vm.VM+(Vmpl1.VM-Vm.VM)*(h-Ym)/(Ympl1-Ym);

            }
            return true;
        }
        catch(...){
            return false;
        }
    }

    //Получает среднеклиматические значения параметры среднего ветра в слое 0-hm,
    //полученные по правилам к ф. 109, п.3.4.
    static bool Get_Vcl_hm(const ClimatData* cl, double hm, ClimatData::Pr &V0hmcl){
        try{
            ClimatData::Pr V0200cl;
            Get_Vcl_h(cl,Constants::level200,V0200cl);
            if (hm>=150){
              V0hmcl=V0200cl;
            }
            else{
              V0hmcl.VZ=V0200cl.VZ*0.82;
              V0hmcl.VM=V0200cl.VM*0.83;
            }
            return true;
        }
        catch(...){
            return false;
        }
    }



//    //Получает коэффициенты линейной автокорреляции между НЕстандартными слоями
//    //из климатической БД
//    static bool Get_Correl_h(double h1, double h2, Correl &Corr){
//        try{
//            int iz=Climat::Zone-1;
//            int im=InData::Tau0::MS0-1;
//            double h;
//            if (h2<h1){
//               h=h1;
//               h1=h2;
//               h2=h;
//            }
//            int is1;
//            int is2;
//            if (h1>Climat::StandartLayer[10]) is1=10;
//            else{
//             is1=Get_is(h1);
//             if (is1<0) is1=Get_is_h(h1);
//            }
//            if (h2>Climat::StandartLayer[11]) is2=11;
//            else{
//             is2=Get_is(h2);
//            if (is2<0) is2=Get_is_h(h2);
//            }
//            Corr.CorrZ=Climat::ClimatCorrel[iz][im][is1][is2].CorrZ;
//            Corr.CorrM=Climat::ClimatCorrel[iz][im][is1][is2].CorrM;;
//            return true;
//        }
//        catch(...){
//            return false;
//        }
//    }

    //Получает коэффициенты линейной автокорреляции между НЕстандартными слоями
    //из климатической БД
    static bool Get_Correl_h_low(const ClimatData* cl, double h1, double h2, ClimatData::Correl &Corr){
        try{
            //int iz=Climat::Zone-1;
           // int im=InData::Tau0::MS0-1;
            double h;
            if (h2<h1){
               h=h1;
               h1=h2;
               h2=h;
            }
            int is1;
            int is2;
            if (h1>Climat::StandartLayer[/*10*/13]) is1=/*10*/13;
            else{
             is1=Get_is(h1);
             if (is1<0) is1=Get_is_h_low(h1);
            }
            if (h2>Climat::StandartLayer[/*11*/14]) is2=/*11*/14;
            else{
             is2=Get_is(h2);
            if (is2<0) is2=Get_is_h_low(h2);
            }
            //!!!cl Corr=cl->corr(Climat::StandartLayer[is1],Climat::StandartLayer[is2]);
             Corr=cl->correl(Climat::StandartLayer[is1],Climat::StandartLayer[is2]);

            return true;
        }
        catch(...){
            return false;
        }
    }

    //Получает коэффициенты линейной автокорреляции между НЕстандартными слоями
    //из климатической БД
    static bool Get_Correl_h_up(const ClimatData* cl, double h1, double h2, ClimatData::Correl &Corr){
        try{
           // int iz=Climat::Zone-1;
          //  int im=InData::Tau0::MS0-1;
            double h;
            if (h2<h1){
               h=h1;
               h1=h2;
               h2=h;
            }
            int is1;
            int is2;
            if (h1>Climat::StandartLayer[/*10*/13]) is1=/*10*/13;
            else{
                if(h1<=Constants::level200) is1=1;
                else{
                   is1=Get_is(h1);
                   if (is1<0) is1=Get_is_h_low(h1);
                }
            }
            if (h2>Climat::StandartLayer[/*11*/14]) is2=/*11*/14;
            else{
             is2=Get_is(h2);
            if (is2<0) is2=Get_is_h(h2);
            }
            //!!!cl Corr=cl->corr(Climat::StandartLayer[is1],Climat::StandartLayer[is2]);
             Corr=cl->correl(Climat::StandartLayer[is1],Climat::StandartLayer[is2]);

            return true;
        }
        catch(...){
            return false;
        }
    }

};

#endif // CLIMAT_H

/*class Sigma;
class Pr;
class Correl;

class ClimatData {
    unsigned int layersCount() const;
    const float* layers() const;
    bool hasLayer(float layer) const;
    const Sigma* sigma(float layer) const;
    const Pr* pr(float layer) const;
    const Correl* sigma(float layer1, float layer2) const;
};*/

