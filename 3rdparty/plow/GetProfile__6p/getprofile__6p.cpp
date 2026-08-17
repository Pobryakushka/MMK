#include "getprofile__6p.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVdVsr_0_200m__3p/paramsvdvsr_0_200m__3p.h"
#include "ParamsVsr_Yl__5p/paramsvsr_yl__5p.h"

#include <iostream>

OutData::ProfileLayer GetProfile__6p::ProfileV[Constants::numStL_out];

GetProfile__6p::GetProfile__6p()
{

}

void GetProfile__6p::Do_6p_after2p(OutData::V* VsrYl,float *realWind, float *averageWind){

    for(int i=0;i<Constants::LOW_NUM_DATA;i++){
       realWind[i]=Constants::UNDEF_TETA;
       averageWind[i]=Constants::UNDEF_TETA;
    }
    for(int i=Constants::LOW_NUM_DATA;i<(2*Constants::LOW_NUM_DATA);i++){
       realWind[i]=Constants::UNDEF_V;
       averageWind[i]=Constants::UNDEF_V;
    }
    GetProfile(VsrYl[1], averageWind[5+Constants::LOW_NUM_DATA], averageWind[5]);//
    GetProfile(VsrYl[2], averageWind[7+Constants::LOW_NUM_DATA], averageWind[7]);
    GetProfile(VsrYl[3], averageWind[9+Constants::LOW_NUM_DATA], averageWind[9]);
    GetProfile(VsrYl[4], averageWind[11+Constants::LOW_NUM_DATA], averageWind[11]);
    GetProfile(VsrYl[5], averageWind[15+Constants::LOW_NUM_DATA], averageWind[15]);
    GetProfile(VsrYl[6], averageWind[17+Constants::LOW_NUM_DATA], averageWind[17]);
    GetProfile(VsrYl[7], averageWind[19+Constants::LOW_NUM_DATA], averageWind[19]);
    GetProfile(VsrYl[8], averageWind[21+Constants::LOW_NUM_DATA], averageWind[21]);
    GetProfile(VsrYl[9], averageWind[22+Constants::LOW_NUM_DATA], averageWind[22]);
    GetProfile(VsrYl[10], averageWind[24+Constants::LOW_NUM_DATA], averageWind[24]);
    GetProfile(VsrYl[11], averageWind[26+Constants::LOW_NUM_DATA], averageWind[26]);
    GetProfile(VsrYl[12], averageWind[28+Constants::LOW_NUM_DATA], averageWind[28]);
    GetProfile(VsrYl[13], averageWind[30+Constants::LOW_NUM_DATA], averageWind[30]);
    GetProfile(VsrYl[14], averageWind[32+Constants::LOW_NUM_DATA], averageWind[32]);

  //-------------------
//    Do_6p();


}

void GetProfile__6p::Do_6p(){
  GetProfileLayer();
  //-------------------
  //Для проверки
  std::cout<<std::endl;
  std::cout<<"---------p.6----------"<<std::endl;
  std::cout<<"--------OutData-----------"<<std::endl;
  std::cout<<"-Vd-"<<std::endl;
   for(int i=0;i<Constants::numStL_out;i++){
     std::cout<<ProfileV[i].layer<<"\t"<<ProfileV[i].Pd.V<<"\t"<<ProfileV[i].Pd.TETA<<std::endl;
   }
   std::cout<<"-Vsr-"<<std::endl;
    for(int i=0;i<Constants::numStL_out;i++){
      std::cout<<ProfileV[i].layer<<"\t"<<ProfileV[i].Psr.V<<"\t"<<ProfileV[i].Psr.TETA<<std::endl;
    }
  //-------------------
}

void GetProfile__6p::GetProfileLayer(){
    ProfileV[0].layer=25;
    ProfileV[1].layer=50;
    ProfileV[2].layer=75;
    ProfileV[3].layer=100;
    ProfileV[4].layer=150;
    ProfileV[5].layer=200;
    for(int i=6;i<Constants::numStL_out;i++){
        ProfileV[i].layer=Constants::StandartLayer[i-5];
    }

    ProfileV[0].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V025_3p.Vd);
    ProfileV[0].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V025_3p.Vsr);
    ProfileV[1].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V050_3p.Vd);
    ProfileV[1].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V050_3p.Vsr);
    ProfileV[2].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V075_3p.Vd);
    ProfileV[2].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V075_3p.Vsr);
    ProfileV[3].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V0100_3p.Vd);
    ProfileV[3].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V0100_3p.Vsr);
    ProfileV[4].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V0150_3p.Vd);//?????????
    ProfileV[4].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V0150_3p.Vsr);
    ProfileV[5].Pd=GetProfile(ParamsVdVsr_0_200m__3p::V0200_3p.Vd);//?????????
    ProfileV[5].Psr=GetProfile(ParamsVdVsr_0_200m__3p::V0200_3p.Vsr);
    for(int i=6;i<Constants::numStL_out;i++){
        ProfileV[i].Pd=GetProfile(ParamsVsr_Yl__5p::V0Yl[i-5].Vd);
        ProfileV[i].Psr=GetProfile(ParamsVsr_Yl__5p::V0Yl[i-5].Vsr);
    }
}

OutData::Profile GetProfile__6p::GetProfile(OutData::V V){
    OutData::Profile Pr;
    if ((V.VZ!=Constants::UNDEF_V)&&(V.VM!=Constants::UNDEF_V)){
        FunctionsCalc::Get_VZ_VM_inv(V,Pr.V,Pr.TETA);
    }
    return Pr;
}

void GetProfile__6p::GetProfile(OutData::V V, float &Vpr, float &TETA){
    Vpr=Constants::UNDEF_V; TETA=Constants::UNDEF_TETA;
    double v,t;
    if((V.VZ!=Constants::UNDEF_V)&&(V.VM!=Constants::UNDEF_V)){
        FunctionsCalc::Get_VZ_VM_inv(V,v,t);
        Vpr=v; TETA=t;
        cout << Vpr << " " << TETA << endl;
    }
}
