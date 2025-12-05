#include "store.h"
#include "storetypes.h"
#include <stdio.h>
#include <string.h>
//#include <qdatetime.h>
//#include "../utils/meteoparser.h"
#include <iostream>

Store::Store() {
    requestData.realWind.setRealWindHeights();
    requestData.averageWind.setAverageWindHeights();
}

Store::~Store() {
}

void Store::swapBytes(char* bytes, int size) {
    char c;
    for (int i = 0; i < size/2; i++) {
        c = bytes[i];
        bytes[i] = bytes[size-i-1];
        bytes[size-i-1] = c;
    }
}
