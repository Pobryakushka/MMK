#include "Constants.h"
#include "indataclimat.h"

double Constants::k0[Climat::numStL]=
 { 0, 1.48, 1.33, 1.33, 1.33, 1.30, 1.25, 1.19, 1.19, 1.19, 1.19, 1.19, 1.19, 1.19, 1.19};
double Constants::StandartLayer[Constants::numStL]=
 { 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0, 1400.0, 1600.0,
   1800.0, 2000.0, 2200.0, 2400.0, 2600.0, 2700.0, 3000.0, 3500.0, 4000.0, 4500.0, 5000.0, 5500.0, 6000.0, 7000.0, 8000.0};
double Constants::StandartLayerMB[Constants::numStL_MB]=
 { 200.0, 400.0, 800.0, 1200.0, 1600.0,
   2000.0, 2400.0, 3000.0, 4000.0, 5000.0, 6000.0, 8000.0};

Constants::Constants()
{

}
