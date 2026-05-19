#ifndef MBULLETIN_H
#define MBULLETIN_H

struct ProfMB{
   /*static*/ double h;//высота, м
   /*static*/ double V;//скорость, м/с
   /*static*/ double TETA;//направление, град
};

struct MBulletin  {
    //    MBulletin() : ProfMeteo(nullptr), layersCount(0) {}
    //    MBulletin(int LaersCount) : ProfMeteo(nullptr), layersCount(0) {
    //        setupHeightsCount(LayersCount);
    //    }
    MBulletin() : ProfMeteo(NULL), layersCount(0) {
        Hm=-1;
        Hd=-1;
        checkm=true;
    }
    ~MBulletin() {
        delete [] ProfMeteo;
    }
    void setupLayersCount(int count) {
        if (layersCount == count)
            return;
        if (ProfMeteo)
            delete [] ProfMeteo;
        ProfMeteo = new ProfMB[count];
        layersCount = count;
    }

    double Hm/*=-1*/;//высота метеостанции, м
    double Hd/*=-1*/;//высота достигнутая зондирования, м
    struct TauM{
        int GD0;
        int MS0;
        int DN0;
        double CH0;//время суток, часы
    }tauM ;
    ProfMB *ProfMeteo;//[Constants::numStL];
    int layersCount;
    bool checkm/*=true*/;//true-метеобюллетень годен для использования

};

#endif // MBULLETIN_H
