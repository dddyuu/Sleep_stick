#ifndef PROTOCOLSA_H
#define PROTOCOLSA_H

#include "protocols.h"
#define FRAMELEN 2080
class ProtocolsA:public Protocols
{
public:
    ProtocolsA();
    int findBufferHead(QByteArray) override;
    int findBufferEnd(QByteArray) override;
    QList<double> analysisEEGdata(QByteArray) override;
    quint16 getFrameLen();
    quint8 getChannelNum();
    // Í¨¹ý Protocols ¼Ì³Ð
    QList<double> analysisEEGdata(QByteArray, QList<double>& data, bool decode_status) override;
private:
    quint8 channel_num=64;
    typedef union {
        char buffer[4];
        quint32 value;
    }ReceiveData;

    
};

#endif // PROTOCOLSA_H
