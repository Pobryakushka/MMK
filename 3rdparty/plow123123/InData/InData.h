//------------------------------------------------------------
//InData
//
// Исходные данные
//------------------------------------------------------------
#ifndef INDATA_H
#define INDATA_H
#include "OutData/outdata.h"
#include "math.h"
#include <math.h>
#include "InData/Constants.h"
#include "stdlib.h"
#include "mhn/structures.h"
#include <vector>

class ProfilePoint;
class MBulletin;

class InData{
public:
 static std::vector <PlowAlgoritm::ProfilePoint> ProfRPV_table;

////Геодезические координаты расчетной точки
// static double B/*=55.00*/;//широта, градусы
// static double L/*=37.00*/;//долгота, градусы
 static double H/*=120*/;//высота, м

 //Расчетный момент времени
 struct Tau0{
     static  int GD0;
     static  int MS0;
     static  int DN0;
     static  double CH0;//время суток, часы
 } ;
 static struct Tau0 tau0;

  /* struct TauM{
      static  int GD0;
      static  int MS0;
      static  int DN0;
      static  double CH0;//время суток, часы
  } ;*/
  //Приближенная оценка параметра шероховатости подстилающей поверхности
  //в точке расчета, м (задается оператором)
  static double z0/*=0.01*/;

  //Параметры приземного ветра
//  static constexpr double V_bottomLayer_BOUNDS=1.5;
  static constexpr double V_bottomLayer_BOUNDS=0.5;
  static double h0/*=4*/;//высота определения параметров приземного ветра, м
  static double V0_tau0/*=0.9*/;//скорость, м/с
  static double TETA0_tau0/*=16.4*/;//направление, град
  static bool bottomLayer;//Имеются ли в наличии данные приземных измерений? true - да, false - нет
  static OutData::V V0ZM;

  //Количество уровней измерений локатора
  static  int k;
//  static const int kk/*=96*/;
 // static int kk1;

  //Вертикальный профиль измеренных параметров действительного ветра
  //в расчетной точке в момент времени tau0
// struct ProfilePoint{
//    double height;//высота, м
//    double speed;//скорость, м/с
//    double azimut;//направление, град
//    int curr;//0-недействительноеб 1-действительное
//};


//static ProfilePoint ProfRPV_table [];

 /////Предыдущие зондирования
 //Признак учета (true)или неучета(false)предыдущих зондирований
 static bool prevZond;
 //Количество предыдущих зондирований(кол-во метеобюллетеней)
 static /*const*/ int kprev;
// struct ProfMB{
//    /*static*/ double h;//высота, м
//    /*static*/ double V;//скорость, м/с
//    /*static*/ double TETA;//направление, град
//} MBprof;
// ProfMB MBprof;
 //Метеобюллетень
// struct MBulletin{
//    double Hm/*=-1*/;//высота метеостанции, м
//    double Hd/*=-1*/;//высота достигнутая зондирования, м
//    struct TauM{
//       int GD0;
//       int MS0;
//       int DN0;
//       double CH0;//время суток, часы
//   }tauM ;
//    ProfMB ProfMeteo[Constants::numStL];
//    bool checkm/*=true*/;//true-метеобюллетень годен для использования
//    MBulletin(){
//       Hm=-1;
//       Hd=-1;
//       checkm=true;
//    }
//};
//static MBulletin MBulletin_table[];
static std::vector <MBulletin> MBulletin_table;

static double LimitTimeMB;//Предел давности для метебюллетеня

 public:
  InData();
  bool Get_ProfRPV_table(PlowAlgoritm::ProfilePoint *profTest, int k, char *fileName);
  bool Get_MBulletin_table(MBulletin *MB, int kprev);
  void ReadProfMeteo(char *mas[], int i);

 // bool Get_bottomLayer();
  //получает признак, имеются ли в наличии данные приземных измерений
  //true - имеются, false - не имеются.
  static bool Get_bottomLayer(){
      if (InData::V0_tau0<InData::V_bottomLayer_BOUNDS)
          return false;
      else return true;
  }

  static void GetTau0(int GD0, int MS0, int DN0, double CH0){
            InData::tau0.GD0=GD0;
            InData::tau0.MS0=MS0;
            InData::tau0.DN0=DN0;
            InData::tau0.CH0=CH0;

//      InData::tau0.GD0=/*2016*/2008/*2014*//*1997*/;
//      InData::tau0.MS0=/*2*/8/*8*//*9*//*1*/;
//      InData::tau0.DN0=/*20*/26/*11*//*8*//*24*/;
//      InData::tau0.CH0=9.717/*0*//*13.183*/;

//      //Чита
//      InData::tau0.GD0=2016;
//      InData::tau0.MS0=/*7*//*1*//*10*/4;
//      InData::tau0.DN0=20/*5*/;
//      InData::tau0.CH0=12/*0*/;

//            //Анадырь
//            InData::tau0.GD0=1996;
//            InData::tau0.MS0=1;
//            InData::tau0.DN0=5;
//            InData::tau0.CH0=0;

//      //Котельный
//      InData::tau0.GD0=2016;
//      InData::tau0.MS0=1;
//      InData::tau0.DN0=15;
//      InData::tau0.CH0=0;

//      //Салехард
//      InData::tau0.GD0=2014;
//      InData::tau0.MS0=4;
//      InData::tau0.DN0=12;
//      InData::tau0.CH0=12;

//      //Тегеран
//      InData::tau0.GD0=2014;
//      InData::tau0.MS0=7;
//      InData::tau0.DN0=8;
//      InData::tau0.CH0=23;

//      //Берген
//      InData::tau0.GD0=2016;
//      InData::tau0.MS0=1;
//      InData::tau0.DN0=20;
//      InData::tau0.CH0=11;
  }

  //Переводит часы и минуты в часы
  static double Get_CH(double hour, double minutes){
      return hour+minutes/60;
  }

  //Переводит текущую дату Т в часы
  static double GetCHfromDate(int GD0,int MS0,int DN0, double CH0/*Tau  T*/){
      double CH;
      int GP=GD0-1;//прошлый год
      div_t n;
      n = div(GP,4);
      //кол-во високосных лет
      int vG=n.quot;
      //кол-во невисокосных лет
      int nvG=GD0-vG;
      bool v=false;//признак високосности текущего года
      if ((GD0%4)==0)
          v=true;
      int MP=MS0-1;//номер предыдущего месяца
      int DP=0;//Дни за предыдущие месяцы в текущем году
      for (int i=1;i<=MP;i++)
       DP+=GetDfromM(i,v);
      CH=((vG*366+nvG*365)+DP+(DN0-1))*24+CH0;
      return CH;
  }

  //Получает количество дней в месяце номер M;
  //v - признак високозности года (true - год високосный)
  static double GetDfromM(int M, bool v){
      switch (M) {
      case 1:case 3:case 5:case 7:
      case 8:case 10:case 12:
          return 31;
      case 2:{
          if (v) return 29;
          return 28;
      }
      case 4:case 6:case 9:case 11:
           return 30;
      default:
          return 0;
      }
  }

  //Получает зональную и меридиальную составляющие (2, 38)
  //V (м), TETA (градусы)
  static OutData::V Get_V0ZM()
  {
      try{
          OutData::V V0ZM;
          V0ZM.VZ=-V0_tau0*sin(TETA0_tau0*M_PI/180);
          V0ZM.VM=-V0_tau0*cos(TETA0_tau0*M_PI/180);
          return V0ZM;
      }
      catch(...){ }
  }

};
#endif // INDATA_H
