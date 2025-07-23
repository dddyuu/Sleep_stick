#ifndef PROTOCOLSB_H
#define PROTOCOLSB_H

#include <QWidget>
#include "protocols.h"

//是否保存原始数据
#define SAVE_RAW_DATA 0
#define SAVE_DATA_LEN 10
extern "C" {

#include "sms4.h"

}
#define FRAMELEN 1030
class ProtocolsB : public Protocols
{
    typedef  union data_transformation
    {
        quint8 buffer[4];
        quint32 data;
    }DataTransformation;
public:
    ProtocolsB();
    int findBufferHead(QByteArray)override;
    int findBufferEnd(QByteArray)override;
    QList<double> analysisEEGdata(QByteArray)override;
    QList<double> analysisEEGdata(QByteArray,QList<double> &data,bool decode_status);

    quint16 getFrameLen()override;
    quint8 getChannelNum()override;
private:
    quint8 channel_num=32;
    //缓存数据不足一帧
    quint16 data_index=0;

#if SAVE_RAW_DATA
    quint8 *save_data=(quint8 *)malloc(1030*SAVE_DATA_LEN);
    int current_point=0;
#endif

};

#endif // PROTOCOLSB_H
