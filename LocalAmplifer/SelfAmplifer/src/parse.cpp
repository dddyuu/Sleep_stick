#include "parse.h"
#include "Protocols/protocolsa.h"
#include "QDebug"
Parse::Parse()
{
    protocols=new ProtocolsA;
}

Parse::~Parse()
{
    status=false;
    QThread::msleep(10);
}

void Parse::start()
{
    status=true;
    QThread::start();
}

void Parse::append(QByteArray data)
{
    mutex.lock();
    this->data.append(data);
    mutex.unlock();
}

quint8 Parse::getChannelNum()
{
    return protocols->getChannelNum();
}

void Parse::setDecodeStatus(bool status)
{
    decode_status=status;
}
void Parse::run()
{
    while (status) {
        mutex.lock();
        for (int i = 0; i < 100; i++)
        {
            if(data.size()<FRAMELEN+2)
            {
                break;
            }
            int head_res=protocols->findBufferHead(data);
            if(head_res==-1)
            {
                data.clear();
            }
            else {
                data.remove(0,head_res);
                int end_res=protocols->findBufferEnd(data);
                if(end_res==0)
                {
                    data.remove(0,1);
                }
                if(end_res==1)
                {
                    quint16 frame_len=protocols->getFrameLen();
                    QByteArray frame=data.mid(0,frame_len);
                    QList<double> raw_channel_data;
                    QList<double> channel_data=protocols->analysisEEGdata(frame,raw_channel_data,decode_status);
                    data.remove(0,frame_len);
                    emit parseFinished(channel_data,raw_channel_data);
                }
            }
        }
        mutex.unlock();
        this->usleep(1);
    }
}
