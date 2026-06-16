#ifndef MATHFUNC_H
#define MATHFUNC_H

#include <math.h>

class MathFunc
{
public:
    struct Rank{
        double mean;
        int ind;
    };
public:
    MathFunc();

    //Поиск минимального значения в массиве среди абсолютных величин
    static bool Poisk_min_abs(double mas[],int lenght, int &i_min)
    {
        try
        {
            double min=0;
            i_min=0;
            //int lenght=sizeof(mas)/sizeof(double);
            for (int i=0;i<lenght;i++)
                if (mas[i]>0)
                {
                   min=mas[i];
                   i_min=i;
                   break;
                }
             for (int i=0;i<lenght;i++)
                 if (mas[i]>0)
                     if (mas[i]<min)
                     {
                         min=mas[i];
                         i_min=i;
                         break;
                     }

            return true;
        }
        catch(...)
        {
            return false;
        }
    }

    //Поиск максимального значения в массиве mas
  static  int Poisk_max(int *mas, int numC, int &i_max){
      double max=mas[0];
      i_max=0;
       for (int i=1;i<numC;i++)
           if (mas[i]>max){
             max=mas[i];
             i_max=i;
           }
      return max;
    }

  //Поиск максимального значения в массиве mas
static  double Poisk_maxd(double *mas, int numC, int &i_max){
    double max=mas[0];
    i_max=0;
     for (int i=1;i<numC;i++)
         if (mas[i]>max){
           max=mas[i];
           i_max=i;
         }
    return max;
  }

//Поиск минимального значения в массиве mas
static  double Poisk_mind(double *mas, int numC, int &i_min){
  double min=mas[0];
  i_min=0;
  for (int i=1;i<numC;i++)
       if (mas[i]<min){
         min=mas[i];
         i_min=i;
       }
  return min;
}

    //Ранжирование по возрастанию массива
    static void RankingIncrMas1(double *mas, int n,Rank *masRank){
       // Rank masRank[n];
        for (int i=0;i<n;i++){
            masRank[i].mean=mas[i];
            masRank[i].ind=i;
        }
        double _mean;
        int _ind;
        for (int j=0;j<n;j++){
            for(int i=0;i<(n-1);i++){
               if (masRank[i+1].mean<masRank[i].mean){
                   _mean=masRank[i].mean;
                   _ind=masRank[i].ind;
                   masRank[i].mean=masRank[i+1].mean;
                   masRank[i].ind=masRank[i+1].ind;
                   masRank[i+1].mean=_mean;
                   masRank[i+1].ind=_ind;
               }
              }
            }
       //return *masRank;
    }

    //Ранжирование по убыванию массива
    static void RankingDecrMas1(double *mas, int n,Rank *masRank){
       // Rank masRank[n];
        for (int i=0;i<n;i++){
            masRank[i].mean=mas[i];
            masRank[i].ind=i;
        }
        double _mean;
        int _ind;
        for (int j=0;j<n;j++){
            for(int i=0;i<(n-1);i++){
               if (masRank[i+1].mean>masRank[i].mean){
                   _mean=masRank[i].mean;
                   _ind=masRank[i].ind;
                   masRank[i].mean=masRank[i+1].mean;
                   masRank[i].ind=masRank[i+1].ind;
                   masRank[i+1].mean=_mean;
                   masRank[i+1].ind=_ind;
               }
              }
            }
       //return *masRank;
    }

};

#endif // MATHFUNC_H
