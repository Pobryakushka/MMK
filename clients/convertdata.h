#ifndef CONVERTDATA_H
#define CONVERTDATA_H

class ConvertData {
public:
    static void convertBytes(char *data, unsigned int size_in_char);
    static void convertBytes(short *data);
    static void convertBytes(int *data);
    static void convertBytes(float *data);
    //    static void convertBytesInMassive(short* data, unsigned int size_in_short);
    //    static void convertBytesInMassive(int* data, unsigned int size_in_int);
    static void convertBytesInMassive(float* data, unsigned int size_in_float);
};

#endif // CONVERTDATA_H
