#ifndef PROTOCOLS_H
#define PROTOCOLS_H
#include "QObject"
class Protocols
{
public:
    virtual int findBufferHead(QByteArray)=0;
    virtual int findBufferEnd(QByteArray)=0;
    virtual QList<double> analysisEEGdata(QByteArray)=0;
    virtual QList<double> analysisEEGdata(QByteArray,QList<double> &data,bool decode_status)=0;
    virtual quint16 getFrameLen()=0;
    virtual quint8 getChannelNum()=0;
};
#endif // PROTOCOLS_H
