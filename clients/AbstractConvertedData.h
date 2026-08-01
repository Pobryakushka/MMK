#ifndef ABSTRACTCONVERTEDDATA_H
#define ABSTRACTCONVERTEDDATA_H

class AbstractAnswerConvertedData {
public:
    AbstractAnswerConvertedData(unsigned int dataSize);
    virtual ~AbstractAnswerConvertedData();
    unsigned char* data() const;

protected:
    virtual unsigned char* dataCore() const = 0;
    virtual unsigned char* convertedDataCore() const = 0;
    unsigned char *m_data;
    const unsigned int m_dataSize;
};

class AbstractRequestConvertData {
public:
    void setData(unsigned char *data, unsigned int size);
protected:
    virtual void setDataCore(unsigned char *data, unsigned int size) = 0;
    virtual void convertDataCore() = 0;
};

#endif // ABSTRACTCONVERTEDDATA_H
