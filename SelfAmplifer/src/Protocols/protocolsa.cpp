#include "protocolsa.h"

ProtocolsA::ProtocolsA()
{

}

int ProtocolsA::findBufferHead(QByteArray data)
{
    QByteArray packet_head;
    packet_head.append(0xA1);
    packet_head.append(0x05);
    return data.indexOf(packet_head);
}

int ProtocolsA::findBufferEnd(QByteArray data)
{
    QByteArray packet_head;
    packet_head.append(0xA1);
    packet_head.append(0x05);
    int frame_len=data.indexOf(packet_head,2);
    if(frame_len>0)
    {
        if(frame_len==FRAMELEN)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

QList<double> ProtocolsA::analysisEEGdata(QByteArray data)
{
   QByteArray data_frame=data.right(96);
   QList<double> channel_data;
   unsigned char temp;
   for(int i=0;i<channel_num;i++)
   {
       
       quint32 value=quint16(data_frame[i*3+1]<<8)+data_frame[i*3+2];
       quint32 high_value=data_frame[i*3]<<24;
       value=(high_value>>8)+value;
       double real_value=(value-8388608)/(8388608*1.0)*5000000/24;
       channel_data.append(real_value);
   }
   return channel_data;
}

quint16 ProtocolsA::getFrameLen()
{
    return 103;
}

quint8 ProtocolsA::getChannelNum()
{
    return channel_num;
}
