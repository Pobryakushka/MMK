#include "AbstractConvertedData.h"

AbstractAnswerConvertedData::AbstractAnswerConvertedData(unsigned int dataSize) : m_dataSize(dataSize)
{
    if (dataSize)
        m_data = new unsigned char[m_dataSize];
}

AbstractAnswerConvertedData::~AbstractAnswerConvertedData()
{
    if (m_dataSize)
        delete [] m_data;
}

unsigned char *AbstractAnswerConvertedData::data() const {
#ifndef MIPS
    return dataCore();
#else
    return convertedDataCore();
#endif
}

//void AbstractAnswerConvertedData::converData()
//{

//}



void AbstractRequestConvertData::setData(unsigned char *data, unsigned int size) {
    setDataCore(data, size);
#ifdef MIPS
    convertDataCore();
#endif
}
