#include "convertdata.h"

void ConvertData::convertBytes(char *data, unsigned int size_in_char)
{
    char c;
    for (unsigned int i = 0; i < size_in_char/2; i++) {
        c = data[i];
        data[i] = data[size_in_char-i-1];
        data[size_in_char-i-1] = c;
    }
}

void ConvertData::convertBytes(short *data)
{
    convertBytes(reinterpret_cast<char*>(data), sizeof(short));
}

void ConvertData::convertBytes(int *data)
{
    convertBytes(reinterpret_cast<char*>(data), sizeof(int));
}

void ConvertData::convertBytes(float *data)
{
    convertBytes(reinterpret_cast<char*>(data), sizeof(float));
}

void ConvertData::convertBytesInMassive(float *data, unsigned int size_in_float)
{
    for (unsigned int i = 0; i < size_in_float; i++) {
        convertBytes(&data[i]);
    }
}
