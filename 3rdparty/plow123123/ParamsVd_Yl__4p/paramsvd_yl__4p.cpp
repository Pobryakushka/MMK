#include "paramsvd_yl__4p.h"

#include "ParamsVd_Yl__4p/ParamsVdVsr_Hl__4_1p/paramsvdvsr_hl__4_1p.h"
#include "FunctionsCalc/functionscalc.h"
#include "ParamsVd_Yl__4_2p/paramsvd_yl__4_2p.h"
#include "ParamsdelV__4_3p/paramsdelv__4_3p.h"
#include "ParamsdelV__4_6p/paramsdelv__4_6p.h"
#include "ParamsVd_Yl__4p/ParamsVYl__4_7p/paramsvyl__4_7p.h"
#include "ParamsMatrix__4_4p/paramsmatrix__4_4p.h"
#include "ParamsMatrix__4_4p/paramsmatrix__4_4p.h"
#include "ParamsVd_Yl__4p/SelectMod__4_5p/selectmod__4_5p.h"

#include <iostream>


double ParamsVd_Yl__4p::Hlmas[Constants::numStL];
OutData::VLayer ParamsVd_Yl__4p::V0Hl[Constants::numStL/*12*/];
int ParamsVd_Yl__4p::INT=/*0*/-1;
double ParamsVd_Yl__4p::HH=0;
OutData::VLayer ParamsVd_Yl__4p::V0HH;

ParamsVdVsr_Hl__4_1p paramsVdVsr_Hl__4_1p;
ParamsVd_Yl__4_2p paramsVd_Yl__4_2p;
ParamsdelV__4_3p paramsdelV__4_3p;
ParamsdelV__4_6p paramsdelV__4_6p;
ParamsVYl__4_7p paramsVYl__4_7p;
ParamsMatrix__4_4p paramsMatrix__4_4p;
SelectMod__4_5p selectMod__4_5p;


ParamsVd_Yl__4p::ParamsVd_Yl__4p()
{

}

bool ParamsVd_Yl__4p::Do_4p(PlowAlgoritm::ProfilePoint* ProfRPV,const ClimatData* cl, int l,
                            OutData::V &VYl,OutData::V **VCast, OutData::V &delV0Yl0Hl){
    try{
        ParamsMatrix__4_4p::M **MZ,**MM;
        double Hl;
        OutData::VLayer V0Hl;
        double Yl=Constants::StandartLayer[l-1];
        paramsVdVsr_Hl__4_1p.Do_4_1p(ProfRPV,cl,l,Hl,V0Hl);
        int m,mpl1;
        int iHl,ihn;
        int NN=paramsMatrix__4_4p.GetNN(ProfRPV, Hl,iHl,ihn);
        MZ=new ParamsMatrix__4_4p::M * [ParamsMatrix__4_4p::numMod];
        MM=new ParamsMatrix__4_4p::M * [ParamsMatrix__4_4p::numMod];
        for (int i=0;i<ParamsMatrix__4_4p::numMod;i++){
            MZ[i]=new ParamsMatrix__4_4p::M[NN];
        }
        for (int i=0;i<ParamsMatrix__4_4p::numMod;i++){
            MM[i]=new ParamsMatrix__4_4p::M[NN];
        }
        //Возможен ли расчет на Yl путем линейной интерполяции
       bool b1=FunctionsCalc::PoiskBoundsLevels(ProfRPV,Yl,m,mpl1);
       bool b2=FunctionsCalc::ProverkaBoundsLevels(ProfRPV,Constants::level200,m,mpl1);
       if (b1 && b2){
                VYl=paramsVd_Yl__4_2p.GetVdYl(ProfRPV,m,mpl1,Yl);
                INT=1;
                //Переход к блоку 5
            }
            else{
                INT=0;
                int kmod;
                if(l<=6/*l==2*/){
                    paramsdelV__4_3p.GetdelV(ProfRPV,cl,l,Yl,Hl,V0Hl.Vsr,VCast,kmod);//4.3
                    delV0Yl0Hl=paramsdelV__4_6p.GetdelV(kmod,true,1,MZ,MM);
                    VYl=paramsVYl__4_7p.GetVYl(Yl,Hl,V0Hl,delV0Yl0Hl);
                    //Переход к блоку 5
                }
                if (l>6/*l>2*/){
                   paramsMatrix__4_4p.GetFormMatrix(ProfRPV,cl,l,Yl,Hl,V0Hl,VCast,NN,iHl,ihn,MZ,MM,kmod); //4.4
                 //  std::cout<<"ParamsVd_Yl__4p::Hlmas[l-1]"<<"\t"<<ParamsVd_Yl__4p::Hlmas[l-1]<<std::endl;
                   Hl=ParamsVd_Yl__4p::Hlmas[l-1];//??????????????
                   V0Hl=ParamsVd_Yl__4p::V0Hl[l-1];
                   // ParamsVd_Yl__4p::Hlmas[l-1]=Hl;//??????????????
                   // ParamsVd_Yl__4p::V0Hl[l-1]=V0Hl;
                  //  std::cout<<"ParamsVd_Yl__4p::Hlmas[l-1]"<<"\t"<<ParamsVd_Yl__4p::Hlmas[l-1]<<std::endl;
                   if (NN>1)
                      selectMod__4_5p.Get3Mod(/*kmod,*/NN,MZ,MM);//4.5
                   delV0Yl0Hl=paramsdelV__4_6p.GetdelV(kmod,false,NN,MZ,MM);;//4.6
                   std::cout<<"ParamsVd_Yl__4p::Hlmas[l-1]"<<"\t"<<ParamsVd_Yl__4p::Hlmas[l-1]<<std::endl;
                   VYl=paramsVYl__4_7p.GetVYl(Yl,Hl,V0Hl,delV0Yl0Hl);//4.7
                  // std::cout<<"ParamsVd_Yl__4p::Hlmas[l-1]"<<"\t"<<ParamsVd_Yl__4p::Hlmas[l-1]<<std::endl;
                   //Для проверки

                   std::cout<<std::endl;
                   std::cout<<"VYl.VZ "<<VYl.VZ<<std::endl;
                   std::cout<<"VYl.VM "<<VYl.VM<<std::endl;
                   std::cout<<std::endl;
                   ParamsVd_Yl__4p::Hlmas[l-1]=Hl;
                   ParamsVd_Yl__4p::V0Hl[l-1]=V0Hl;
                   //---------------
              //     ParamsVd_Yl__4p::Hlmas[l-1]=ParamsVd_Yl__4p::HH;//??????????
              //     ParamsVd_Yl__4p::V0Hl[l-1]=ParamsVd_Yl__4p::V0HH;

                    //Переход к блоку 5

                }
            }

       for (int j = 0; j < ParamsMatrix__4_4p::numMod; j++) {
           delete [] MZ[j];
       }
       delete [] MZ;
       for (int j = 0; j < ParamsMatrix__4_4p::numMod; j++) {
           delete [] MM[j];
       }
       delete [] MM;
       return true;
    }
    catch(...){
        return false;
    }
}
