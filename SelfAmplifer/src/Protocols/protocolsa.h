#ifndef PROTOCOLSA_H
#define PROTOCOLSA_H

#include "protocols.h"
#define FRAMELEN 103
class ProtocolsA:public Protocols
{
public:
    ProtocolsA();
    int findBufferHead(QByteArray) override;
    int findBufferEnd(QByteArray) override;
    QList<double> analysisEEGdata(QByteArray) override;
    quint16 getFrameLen();
    quint8 getChannelNum();
private:
    quint8 channel_num=32;
};

#endif // PROTOCOLSA_H
