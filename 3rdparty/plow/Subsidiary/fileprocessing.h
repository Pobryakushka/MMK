
#ifndef FILEPROCESSING_H
#define FILEPROCESSING_H

#include <string.h>
#include <strings.h>

#include <iostream>
#include <fstream>
//#include <string>


class FileProcessing
{
public:
    FileProcessing();
    bool ReadRPVprof(int k, char *mas[], char *fileName);
};

#endif // FILEPROCESSING_H
