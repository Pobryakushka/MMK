#include <iostream>
#include "windshear.h"
#include "math.h"
#include "mhn/structures.h"

WindShear::WindShear(float *windShear)
{

}

WindShear::MessErrors WindShear::GetWindShear(float *windShear){
    try{
    for(int i=0;i<(kwsh-1);i++){
       windShear[i+(kwsh-1)]=GetDelV(ProfRPV[i].speed, ProfRPV[i].azimut, ProfRPV[i+1].speed, ProfRPV[i+1].azimut);
       windShear[i]=GetDelAlf(ProfRPV[i].speed, ProfRPV[i].azimut, ProfRPV[i+1].speed, ProfRPV[i+1].azimut, windShear[i+(kwsh-1)]);
    }
    return RESULT_OK;
    }
    catch(...){
      return WINDSHEAR_NOT_CONFIG_ALG;
    }
}

//Устанавливает параметры профиля ветра
void WindShear::setProfRpv(PlowAlgoritm::ProfilePoint* prof, int hCount){
    ProfRPV=new PlowAlgoritm::ProfilePoint[hCount];
   // std::cout<<"--------ProfRPV-------"<<std::endl;
    for(int i=0;i<hCount;i++){
       ProfRPV[i].height=prof[i].height;
       ProfRPV[i].curr=prof[i].curr;
       if(prof[i].curr==1)
          ProfRPV[i].speed=prof[i].speed;
       else ProfRPV[i].speed=Constants::UNDEF_V;
       ProfRPV[i].azimut=prof[i].azimut;
     //  std::cout<<ProfRPV[i].speed<<std::endl;
    }
    kwsh=hCount;
}

float WindShear::GetDelV(float V1, float alf1, float V2, float alf2){
    alf1*=M_PI/180;
    alf2*=M_PI/180;
    float delV=Constants::UNDEF_V;
    if ((V1==Constants::UNDEF_V)||(V2==Constants::UNDEF_V))
       return Constants::UNDEF_V;
    delV=pow(V1,2)+pow(V2,2)-2*V1*V2*cos(alf1-alf2);
    delV=pow(delV,0.5);
    return delV;
}

float WindShear::GetDelAlf(float V1, float alf1, float V2, float alf2, float delV){
    float delAlf=0;
    if (((alf2-alf1)==0)&&(delV==0))
        return 0;
    float gam=(alf2-alf1)*(180-fabs(alf2-alf1));
    int _sign=sign(gam);
    float a_cos=acos((pow(delV,2)+pow(V2,2)-pow(V1,2))/(2*delV*V2));
    delAlf=a_cos*180/M_PI*_sign+alf2+180;
    if(delAlf>360) delAlf-=360;
    return delAlf;
}

int WindShear::sign(float gam){
    int sign=0;
    return sign = (gam>=0) ? 1 : -1;
}

WindShear::~WindShear()
{
    delete [] ProfRPV;
    //std::cout<<"-----End Program!!!-----------"<<std::endl;
}
