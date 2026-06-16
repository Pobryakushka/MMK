#include "profile.h"
#include "ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h"
#include "ParamsVd_Yl__4p/paramsvd_yl__4p.h"
//#include "InData/InData.h"

#include "CastH__1p/casth__1p.h"
#include "Profile/end.h"
#include "BottomEval__2p/bottomeval__2p.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVsr_Yl__5p/paramsvsr_yl__5p.h"
#include "GetProfile__6p/getprofile__6p.h"
#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"
#include "ParamsVd_Yl__4p/ParamsdelV__4_3p/paramsdelv__4_3p.h"

#include <iostream>

ParamsVdVsr_0_200m__3p paramsVdVsr_0_200m__3p;
ParamsVd_Yl__4p paramsVd_Yl__4p;
CastH__1p castH__1p;
BottomEval__2p bottomEval__2p;
ParamsVsr_Yl__5p paramsVsr_Yl__5p;
GetProfile__6p getProfile__6p;


using namespace std;

ProfileReal::ProfileReal()
{
//  GetProfile(realWind, averageWind);
}

ProfileReal::MessErrors ProfileReal::GetProfile(float *realWind, float *averageWind){
    try{
        if(ProfRPV == nullptr)
            return PROFILE_NOT_CONFIG_PROF_RPV;
        if (cl == nullptr || !cl->isDataReady())
            return PROFILE_CLIMAT_NOT_READY;
        if (InData::H < -1000)
            return PROFILE_NOT_CONFIG_HEIGHT;
    //Массив приведенных значений скорости (по метеобюллетеням)
        OutData::V **VCast;
        VCast=new OutData::V * [InData::kprev];
        if(InData::kprev > 0) {
            for (int i=0;i<InData::kprev;i++){
               VCast[i]=new OutData::V[Constants::numStL_MB];
            }
            castH__1p.GetVCast(cl, VCast);

            //Для проверки
            std::cout<<"-----------p.1-------------"<<std::endl;
            for(int k=0;k<InData::kprev;k++){
                cout << "MeteoBull N"<<k+1 << endl;
                for (int lev=0;lev<Constants::numStL_MB;lev++){
                    cout <<Constants::StandartLayerMB[lev]<<"\t"<<VCast[k][lev].VZ <<"\t"<< VCast[k][lev].VM<< endl;
                }
            }
        }

    //Параметры среднего ветра в стандартных слоях
    OutData::V VsrYl[Climat::numStL];
  //  bottomEval__2p.GetVsr__2p(VsrYl);//!!!Для проверки
    //Если измерения локатора отсутствуют
    if (InData::k<=0){
        if (!InData::bottomLayer)
            cout << "InData::bottomLayer = " << InData::bottomLayer << ", InData::V0_tau0 = " << InData::V0_tau0 << endl;
//           return PROFILE_NOT_CONFIG_PROF_RPV;//Расчеты профиля ветра невозможны
        else
            bottomEval__2p.GetVsr__2p(cl,VsrYl,VCast);//п.2
        getProfile__6p.Do_6p_after2p(VsrYl,realWind,averageWind);
    }
    else{

        paramsVdVsr_0_200m__3p.Do_3p(ProfRPV,cl,VCast);
        int l=0;
        OutData::V Vd_Yl;
        OutData::V delV0Yl0Hl;//приращения, полученные в 4.6

        std::cout<<"p.4"<<std::endl;
        std::cout<<"Yl"<<"\t"<<"VYl.VZ"<<"\t"<<"VYl.VM"<<std::endl;
        for(int i=1/*6*/; i<Constants::numStL/*21*//*25*//*10*//*22*//*8*/;i++){
            std::cout<<std::endl;
            std::cout<<"-----------Level 0-"<<Constants::StandartLayer[i]<<"---------"<<std::endl;
            l=i+1;
//            if(i==15/*10*//*8*//*7*//*26*//*19*/)
//                l=i+1;
            paramsVd_Yl__4p.Do_4p(ProfRPV,cl,l,Vd_Yl,VCast,delV0Yl0Hl);
            std::cout<<Constants::StandartLayer[i]<<"VYl\t"<<Vd_Yl.VZ<<"\t"<<Vd_Yl.VM<<std::endl;
            double Hl=ParamsVd_Yl__4p::Hlmas[l-1];
            OutData::VLayer V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
            paramsVsr_Yl__5p.Do_5p(ProfRPV,/*cl,*/l,Hl,V0Hl,Vd_Yl,ParamsVd_Yl__4p::V0HH,ParamsVd_Yl__4p::HH,delV0Yl0Hl);

            std::cout<<Constants::StandartLayer[i]<<"V0Yl\t "<<ParamsVsr_Yl__5p::V0Yl[l-1].Vsr.VZ<<
                       "\t "<<ParamsVsr_Yl__5p::V0Yl[l-1].Vsr.VM<<std::endl;
            std::cout<<Constants::StandartLayer[i]<<"VYlcorr\t "<<ParamsVsr_Yl__5p::V0Yl[l-1].Vd.VZ<<
                       "\t "<<ParamsVsr_Yl__5p::V0Yl[l-1].Vd.VM<<std::endl;
        }
       getProfile__6p.Do_6p();//
       for(int i=0;i<Constants::LOW_NUM_DATA;i++){
          realWind[i]=GetProfile__6p::ProfileV[i].Pd.TETA;
          averageWind[i]=GetProfile__6p::ProfileV[i].Psr.TETA;
       }
       for(int i=Constants::LOW_NUM_DATA;i<(2*Constants::LOW_NUM_DATA);i++){
          realWind[i]=GetProfile__6p::ProfileV[i-Constants::LOW_NUM_DATA].Pd.V;
          averageWind[i]=GetProfile__6p::ProfileV[i-Constants::LOW_NUM_DATA].Psr.V;
       }
    }
    End(VCast,InData::kprev);
    //---------------------------------
    SetInitial();
    //---------------------------------

    for(int i=Constants::LOW_NUM_DATA;i<(2*Constants::LOW_NUM_DATA);i++) {
        auto iShift = i - Constants::LOW_NUM_DATA;
        if (realWind[i] == Constants::UNDEF_V || realWind[iShift] == Constants::UNDEF_TETA || std::isnan(realWind[i]) || std::isnan(realWind[iShift])) {
            realWind[i] = 0.f;
            realWind[i - Constants::LOW_NUM_DATA] = -9999.0f;
        }
        if (averageWind[i] == Constants::UNDEF_V || averageWind[iShift] == Constants::UNDEF_TETA || std::isnan(averageWind[i]) || std::isnan(averageWind[iShift])) {
            averageWind[i] = 0.f;
            averageWind[i - Constants::LOW_NUM_DATA] = -9999.0f;
        }
    }

    return RESULT_OK;
    }
    catch(...){
        SetInitial();
        return PROFILE_NOT_CONFIG_PROF_RPV;
    }
}

void ProfileReal::SetInitial(){
    OutData::VLayer V;
    ParamsVdVsr_0_200m__3p::V0200_3p=V;
    ParamsVdVsr_0_200m__3p::V0100_3p=V;
    ParamsVdVsr_0_200m__3p::V050_3p=V;
    ParamsVdVsr_0_200m__3p::V025_3p=V;
    ParamsVdVsr_0_200m__3p::V075_3p=V;
    ParamsVdVsr_0_200m__3p::V0150_3p=V;

    ParamsVdVsr_0_200m__3_1p::Mode mode;
    for(int i=0; i<7;i++)
       ParamsVdVsr_0_200m__3_1p::ComplMode[i]=mode;

    FunctionsCalc::INT100=0;

    OutData::ProfileLayer pr;
    for(int i=0; i<Constants::numStL_out;i++)
      GetProfile__6p::ProfileV[i]=pr;

    ParamsVdVsr_Hl__4_1p::IPRL=0;

    ParamsdelV__4_3p::Mode mode1;
    for(int i=0; i<3;i++)
      ParamsdelV__4_3p::ComplMode[i]=mode1;

    ParamsdelV__4_3p::IPRHH=-1;

    for(int i=0;i<Constants::numStL;i++)
      ParamsVd_Yl__4p::Hlmas[i]=0;

    for(int i=0;i<Constants::numStL;i++)
      ParamsVd_Yl__4p::V0Hl[i]=V;

    ParamsVd_Yl__4p::INT=-1;
    ParamsVd_Yl__4p::HH=0;
    ParamsVd_Yl__4p::V0HH=V;

    for(int i=0;i<Constants::numStL;i++)
     ParamsVsr_Yl__5p::V0Yl[i]=V;
}

//Устанавливает параметры профиля ветра
void ProfileReal::setProfRpv(PlowAlgoritm::ProfilePoint *prof, int hCount){
    if (ProfRPV)
        delete [] ProfRPV;
    ProfRPV=new PlowAlgoritm::ProfilePoint[hCount];
    InData::k=hCount;
  //  InData::ProfRPV_table.reserve(InData::k);
   // InData::ProfRPV_table.resize(InData::k);
  //  std::cout<<"--------ProfRPV-------"<<std::endl;
    for(int i=0;i<hCount;i++){
       ProfRPV[i].height=prof[i].height;
       ProfRPV[i].curr=prof[i].curr;
       if(prof[i].curr==1)
          ProfRPV[i].speed=prof[i].speed;
       else ProfRPV[i].speed=Constants::UNDEF_V;
       ProfRPV[i].azimut=prof[i].azimut;

//       InData::ProfRPV_table[i].height=prof[i].height;
//       InData::ProfRPV_table[i].curr=prof[i].curr;
//       if(prof[i].curr==1)
//          InData::ProfRPV_table[i].speed=prof[i].speed;
//       else InData::ProfRPV_table[i].speed=Constants::UNDEF_V;
//       InData::ProfRPV_table[i].azimut=prof[i].azimut;

       std::cout<< ProfRPV[i].height << " " <<
                   ProfRPV[i].speed << " " << ProfRPV[i].azimut << std::endl;//
    }

}

//Устанавливает параметры метеобюллетеней
void ProfileReal::setMeteoMid(const MBulletin* MB, int size, bool prevzond){
   InData::prevZond=prevzond;
   if(!prevzond)
       InData::kprev=0;
   else InData::kprev=size;
   InData::MBulletin_table.reserve(InData::kprev);
   InData::MBulletin_table.resize(InData::kprev);
   for (int i = 0; i < InData::kprev; i++){
       InData::MBulletin_table[i].setupLayersCount(Constants::numStL);
   }
   for(int i=0;i<InData::kprev;i++){
    InData::MBulletin_table[i].Hm=MB[i].Hm;
    InData::MBulletin_table[i].Hd=MB[i].Hd;
    InData::MBulletin_table[i].checkm=MB[i].checkm;
    InData::MBulletin_table[i].tauM.GD0=MB[i].tauM.GD0;
    InData::MBulletin_table[i].tauM.MS0=MB[i].tauM.MS0;
    InData::MBulletin_table[i].tauM.DN0=MB[i].tauM.DN0;
    InData::MBulletin_table[i].tauM.CH0=MB[i].tauM.CH0;
    for(int s=0;s<Constants::numStL_MB;s++){
        InData::MBulletin_table[i].ProfMeteo[s].h=MB[i].ProfMeteo[s].h;
        InData::MBulletin_table[i].ProfMeteo[s].V=MB[i].ProfMeteo[s].V;
        InData::MBulletin_table[i].ProfMeteo[s].TETA=MB[i].ProfMeteo[s].TETA;
    }

   }
}

//Установка параметров приземного ветра
void ProfileReal::setGroundWind(float speed, float azimut, float height){
    InData::h0=height;
    InData::V0_tau0=speed;
    InData::TETA0_tau0=azimut;
    InData::bottomLayer=InData::Get_bottomLayer();
    InData::V0ZM=InData::Get_V0ZM();
}

void ProfileReal::setHeight(double H)
{
    InData::H=H;
}

//Установка широты, долготы, высоты в градусах
//void ProfileReal::setBLH(double B, double L, double H){
//    InData::B=B;
//    InData::L=L;
//    InData::H=H;
//}

//Установка даты, времени
void ProfileReal::setDateTime(int GD0, int MS0, int DN0, double CH0){

    InData::GetTau0(GD0, MS0, DN0, CH0);
}

/**Установка шероховатости
 * @brief ProfileReal::setz0
 * @param z0 Параметр шероховаости, м
 */
void ProfileReal::setz0(float z0){

    InData::z0=z0;
}

void ProfileReal::End(void *dMasToDel, int numRow){
    OutData::V ** dMas = static_cast<OutData::V **>(dMasToDel);
    for (int j = 0; j < numRow; j++) {
        delete [] dMas[j];
    }
    delete [] dMas;
}

ProfileReal::~ProfileReal()
{
    if (ProfRPV)
        delete [] ProfRPV;
    //std::cout<<"-----End Program!!!-----------"<<std::endl;
}
