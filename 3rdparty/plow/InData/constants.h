#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace PlowAlgoritm {

    class Constants
    {
     public:
     //Уровни стандартной артиллерийской атмосферы
     static constexpr double level25=25;//м
     static constexpr double level50=50;//м
     static constexpr double level75=75;//м
     static constexpr double level100=100;//м
     static constexpr double level150=150;//м
     static constexpr double level200=200;//м
     static constexpr double level400=400;//м
     static constexpr double level500=500;//м
     static constexpr double level8000=8000;//м
     static constexpr double UNDEF_V=999.0;//Неопределенное значение для скорости
     static constexpr double UNDEF_TETA=999.0;//Неопределенное значение для направления
     static constexpr double UNDEF_MeteoB=99;//Неопределенное значение для параметров метеобюллетеня (возможно, изменится)
     static const int numStL=/*12*/28;
     static const int numStL_out=/*14*/33;//Кол-во уровней стандартной артилл. атмосферы (для выходных данных)
     static const int numStL_MB=12;//Кол-во стандартных слоев метеобюллетеня
     //static const int numMB=100;//максимальное кол-во метеобюллетеней
     //Коэфф-ты для оценки средней временной изменчивости
     //ветра в зависимости от сдвига по времени (по ф.26)
     static double k0[] ;

     static const int LOW_NUM_DATA=/*14*/33;//кол-во высот действительного ветра
     static const int REAL_NUM_DATA=/*14*/33;//кол-во высот среднего ветра

     static double  StandartLayer[] ;
      static double  StandartLayerMB[] ;

     public:
       Constants();
    };

    struct ProfilePoint{
       float speed;                //��������
       float azimut;               //����
       float height;               //������
       //bool  curr;                 //���� �������������
       int curr;
    };              //������ ������� ������ � ������� �����
    typedef ProfilePoint* pProfilePoint;
}
#endif // CONSTANTS_H
