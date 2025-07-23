#include "protocolsa.h"
#include "QDebug"
ProtocolsA::ProtocolsA()
{

}

int ProtocolsA::findBufferHead(QByteArray data)
{
    QByteArray packet_head;
    packet_head.append(0xAA);
    packet_head.append(0x55);
    return data.indexOf(packet_head);
}

int ProtocolsA::findBufferEnd(QByteArray data)
{
    QByteArray packet_head;
    packet_head.append(0xAA);
    packet_head.append(0x55);
    if (data.size() < FRAMELEN+2)
    {
        return -1;
    }
    else
    {
        if (data[FRAMELEN] ==char(0xAA) && data[FRAMELEN + 1] == char(0x55))
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
    //int frame_len=data.indexOf(packet_head,2);
    //qDebug() << frame_len;
    //if(frame_len>0)
    //{
    //    if(frame_len==FRAMELEN)
    //    {
    //        return 1;
    //    }
    //    else
    //    {
    //        return 0;
    //    }
    //}
    //else
    //{
    //    return -1;
    //}
}

QList<double> ProtocolsA::analysisEEGdata(QByteArray data)
{
    QList<double> eeg;
    int dataIndex = 11;
    QByteArray frame_data = data.mid(11);
    for (int i = 0; i < channel_num; ++i) {
        ReceiveData receive_data;
        receive_data.buffer[0] = frame_data[i * 3 + 2];
        receive_data.buffer[1] = frame_data[i * 3 + 1];
        receive_data.buffer[2] = frame_data[i * 3];
        receive_data.buffer[3] = 0x00;
        double value = receive_data.value;
        eeg.append(value);

    }
    return eeg;
}

quint16 ProtocolsA::getFrameLen()
{
    return FRAMELEN;
}

quint8 ProtocolsA::getChannelNum()
{
    return channel_num;
}

QList<double> ProtocolsA::analysisEEGdata(QByteArray buffer, QList<double>& data, bool decode_status)
{
    
    QList<double> chart_data;
    for (int index= 0; index < 10; index++)
    {
        QByteArray frame_data = buffer.mid(15);
        buffer.remove(0, 208);
        for (int i = 0; i < channel_num; ++i) {
            ReceiveData receive_data;
            receive_data.buffer[0] = frame_data[i * 3 + 2];
            receive_data.buffer[1] = frame_data[i * 3 + 1];
            receive_data.buffer[2] = frame_data[i * 3];
            receive_data.buffer[3] = 0x00;
            double value = receive_data.value;
            double chart_value = (value - 8388068) * 1.0 / 8388068 * 5000000 / 24;
            chart_data.append(chart_value);
            data.append(value);
        }
    }
    return chart_data;
}
