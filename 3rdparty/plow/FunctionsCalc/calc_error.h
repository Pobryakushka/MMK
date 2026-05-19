#ifndef CALC_ERROR_H
#define CALC_ERROR_H

//#include "math.h"
#include <cmath>
#include "InData/InData.h"
#include "InData/indataclimat.h"
//#include "InData/climatdata.h"
#include "mhn/structures.h"

class Calc_error
{
public:
    //Оценки теоретических ошибок
    struct Eps{
       double EpsZ/*=-1*/;
       double EpsM/*=-1*/;
       Eps(){
           EpsZ=-1;
           EpsM=-1;
       }
   };
    //Весовые коэффициенты
    struct P{
       double PZ/*=0*/;
       double PM/*=0*/;
       P(){
         PZ=0;
         PM=0;
       }
   };
public:
    Calc_error();

    //Получает условный коэффициент корреляции rr (ф. 41)
    //RR -   - условное расстояние (в м), являющееся показателем
    //распределения уровней измерений в пределах задаваемого слоя
    static double Get_rr(double RR){
        return pow(0.96,(RR/100));
    }

    //Получает условное расстояние (в м), являющееся показателем распределения
    //уровней измерений в пределах задаваемого слоя (ф. 42)
    //levelMin,levelMax - нижняя и верхняя границы рассматриваемого слоя
    static double Get_RR(double levelMin, double levelMax, PlowAlgoritm::ProfilePoint *ProfRPV){
        double delH=levelMax-levelMin;//толщина слоя, м
        double hmas[1000];
        hmas[0]=levelMin;
        for (int i=1;i<1000;i++)
            hmas[i]=-1.0;
        int N=0;
        double h=0;
        for (int i=0;i<InData::k;i++){
            h= ProfRPV[i].height;
            if ((h<levelMax) && (h>levelMin)){
                hmas[N+1]=h;
                N++;
            }
            if (h>levelMax)
                break;
        }
        hmas[N+1]=levelMax;
        double R_t=delH/(N+1);
        double sum_R=0.0;
        // double sum=0;
        double del1=0;
        double del2=0;
        for (int i=0;i<=N;i++){
            del1=std::abs((double)(hmas[i+1]-hmas[i]));
            del2=((double)((double)del1-(double)R_t));
            if (del2<0)
               del2*=-1.0;
            sum_R=(double)(sum_R+del2);
        }

        return (double)(R_t+sum_R/(double)(N+1));
    }

    //Получает оценки ошибок ф. 87, ф.40
    static bool Get_Eps(double Si_VZ, double Si_VM, double rr, double &Eps_VZ, double &Eps_VM){
        try{
            Eps_VZ=(double)Si_VZ*sqrt(2.0*(1-rr));
            Eps_VM=(double)Si_VM*sqrt(2.0*(1-rr));
            return true;
        }
        catch(...){
            return false;
        }
    }

    //Получает оценки ошибок ф. 87, 40
    static bool Get_Eps(ClimatData::Sigma Si, double rr, Eps &EpsZM){
        try{
            EpsZM.EpsZ=(double)Si.SiVZ*sqrt(2.0*(1-rr));
            EpsZM.EpsM=(double)Si.SiVM*sqrt(2.0*(1-rr));
            return true;
        }
        catch(...){
            return false;
        }
    }

    //Получает оценки ошибок ф.87, ф.40
    static Eps Get_Eps(ClimatData::Sigma Si, double levelMin, double levelMax,PlowAlgoritm::ProfilePoint* ProfRPV){
        Eps EpsZM;
        try{
            double RR=Get_RR(levelMin,levelMax,ProfRPV);
            double rr=Get_rr(RR);
            EpsZM.EpsZ=(double)Si.SiVZ*sqrt(2.0*(1-rr));
            EpsZM.EpsM=(double)Si.SiVM*sqrt(2.0*(1-rr));
            return EpsZM;
        }
        catch(...){
            return EpsZM;
        }
    }

    //Получает оценки ошибок по обновленной ф.40 new
    static Eps Get_Eps(ClimatData::Sigma Si, double* hmas,double hh ){
        Eps EpsZM;
        try{
            int N=1;
            double Rsr=hh/(N+1);
            double sum=0;
            for(int i=0;i<(N+1);i++)
                sum=fabs(fabs(hmas[i+1]-hmas[i])-Rsr);
            double RR=Rsr+sum/(N+1);//ф.42 new
            double rr=Get_rr(RR);
            EpsZM.EpsZ=(double)Si.SiVZ*sqrt(2.0*(1-rr));
            EpsZM.EpsM=(double)Si.SiVM*sqrt(2.0*(1-rr));
            return EpsZM;
        }
        catch(...){
            return EpsZM;
        }
    }

    //Получает оценки ошибок ф.102
    static Eps Get_Eps_f102(const ClimatData* cl, double Yl, double hm, double hm1){
        Eps EpsZM;
        try{
            double r0=Getr0(Yl, hm, hm1);
            ClimatData::Sigma Si0hm, Si0hm1;
            Climat::Get_Si_h(cl,hm,Si0hm);
            Climat::Get_Si_h(cl,hm1,Si0hm1);
            double siZ=pow(Si0hm.SiVZ,2)+pow(Si0hm1.SiVZ,2);
            double siM=pow(Si0hm.SiVM,2)+pow(Si0hm1.SiVM,2);
            EpsZM.EpsZ=r0/(hm1-hm)*pow(siZ,0.5);
            EpsZM.EpsM=r0/(hm1-hm)*pow(siM,0.5);
            return EpsZM;
        }
        catch(...){
            return EpsZM;
        }
    }

    //Получает значение величины r0 согласно ф.103
    static double Getr0(double Yl, double hm, double hm1){
        double r0;
        if(fabs(Yl-hm)<=fabs(Yl-hm1))
           r0=Yl-hm;
        else r0=hm1-Yl;
        return r0;
    }

};

#endif // CALC_ERROR_H
