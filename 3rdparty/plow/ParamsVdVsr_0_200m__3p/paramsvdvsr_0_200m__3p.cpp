#include "paramsvdvsr_0_200m__3p.h"
#include "InData/Constants.h"
#include "ParamsVdVsr_0_200m__3_4p/paramsvdvsr_0_200m__3_4p.h"

#include "ParamsVdVsr_0_200m__3_2p/paramsvdvsr_0_200m__3_2p.h"
#include "ParamsVdVsr_0_200m__3_3p/paramsvdvsr_0_200m__3_3p.h"
#include "climatdata.h"

#include <iostream>

ParamsVdVsr_0_200m__3_4p paramsVdVsr_0_200m__3_4p;
ParamsVdVsr_0_200m__3_1p paramsVdVsr_0_200m__3_1p;
ParamsVdVsr_0_200m__3_2p paramsVdVsr_0_200m__3_2p;
ParamsVdVsr_0_200m__3_3p paramsVdVsr_0_200m__3_3p;

OutData::VLayer ParamsVdVsr_0_200m__3p::V0200_3p, ParamsVdVsr_0_200m__3p::V0100_3p, ParamsVdVsr_0_200m__3p::V050_3p,
              ParamsVdVsr_0_200m__3p::V025_3p, ParamsVdVsr_0_200m__3p::V075_3p, ParamsVdVsr_0_200m__3p::V0150_3p;

ParamsVdVsr_0_200m__3p::ParamsVdVsr_0_200m__3p()
{

}

bool ParamsVdVsr_0_200m__3p::Do_3p(PlowAlgoritm::ProfilePoint* ProfRPV, const ClimatData* cl, OutData::V **VCast){
    try{
        if ( ProfRPV[0].height>Constants::level200){
          if (InData::bottomLayer)
            paramsVdVsr_0_200m__3_1p.Do_3_1(ProfRPV,cl,ParamsVdVsr_0_200m__3p::V0200_3p,ParamsVdVsr_0_200m__3p::V0100_3p,
              ParamsVdVsr_0_200m__3p::V050_3p, ParamsVdVsr_0_200m__3p::V025_3p,ParamsVdVsr_0_200m__3p::V075_3p, ParamsVdVsr_0_200m__3p::V0150_3p, VCast) ;//3.1
              else
              paramsVdVsr_0_200m__3_2p.Do_3_2(ProfRPV, cl,ParamsVdVsr_0_200m__3p::V0200_3p,ParamsVdVsr_0_200m__3p::V0100_3p,VCast);//3.2
        }
        else{
         if (InData::bottomLayer)
            paramsVdVsr_0_200m__3_3p.Do_3_3(ProfRPV, cl,ParamsVdVsr_0_200m__3p::V0200_3p,ParamsVdVsr_0_200m__3p::V0100_3p,ParamsVdVsr_0_200m__3p::V050_3p,
                                            ParamsVdVsr_0_200m__3p::V025_3p,ParamsVdVsr_0_200m__3p::V075_3p, ParamsVdVsr_0_200m__3p::V0150_3p, VCast);//3.3
          else
            paramsVdVsr_0_200m__3_4p.Do_3_4(ProfRPV,cl,ParamsVdVsr_0_200m__3p::V0200_3p,ParamsVdVsr_0_200m__3p::V0100_3p,ParamsVdVsr_0_200m__3p::V050_3p,
                                            ParamsVdVsr_0_200m__3p::V075_3p, ParamsVdVsr_0_200m__3p::V0150_3p,VCast) ;
        }

        //---------------
        //!!!Для проверки
        std::cout<<"---------------p.3-----------------------"<<std::endl;
        std::cout<<"-----V025"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V025_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V025_3p.Vd.VM<<std::endl;
        std::cout<<"-----V050"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V050_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V050_3p.Vd.VM<<std::endl;
        std::cout<<"Vsr\t"<<ParamsVdVsr_0_200m__3p::V050_3p.Vsr.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V050_3p.Vsr.VM<<std::endl;
        std::cout<<"-----V075"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V075_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V075_3p.Vd.VM<<std::endl;
        std::cout<<"-----V0100"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V0100_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V0100_3p.Vd.VM<<std::endl;
        std::cout<<"Vsr\t"<<ParamsVdVsr_0_200m__3p::V0100_3p.Vsr.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V0100_3p.Vsr.VM<<std::endl;
        std::cout<<"-----V0150"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V0150_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V0150_3p.Vd.VM<<std::endl;
        std::cout<<"-----V0200"<<std::endl;
        std::cout<<"Vd\t"<<ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V0200_3p.Vd.VM<<std::endl;
        std::cout<<"Vsr\t"<<ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VZ<<"\t"<<ParamsVdVsr_0_200m__3p::V0200_3p.Vsr.VM<<std::endl;
        //---------------
        return true;
       }
    catch(...){
           return false;
       }
}
