#include "selfamplifer.h"
#include "QDebug"
SelfAmplifer::SelfAmplifer()
{
    qRegisterMetaType<QList<double>>("QList<double>");
    init();
}

SelfAmplifer::~SelfAmplifer()
{

}

void SelfAmplifer::start()
{
    status=true;
}

void SelfAmplifer::stop()
{
    status=false;
}

QStringList SelfAmplifer::getChannelName()
{
    return {"FP2","AFZ","F4","F8","FCZ","FC4","FT8","CZ","T7","C3","FT7","FC3","F7","F3","FZ","FP1","O2","PO4","P8","P4",
    "TP8","CP4","T8","C4","CP3","TP7","PZ","P3","P7","PO3","OZ","O1"};
}

quint8 SelfAmplifer::getChannnelNum()
{
    return channel_num;
}

quint16 SelfAmplifer::getSampleRate()
{
    return 2000;
}

QWidget *SelfAmplifer::getConnectWidget()
{
    return communication->getConnectWidget();
}

QWidget *SelfAmplifer::getConfigWidget()
{
    return NULL;
}

QList<double> SelfAmplifer::getOneData()
{
    QList<double> data;
    return data;
}

QList<QList<double> > SelfAmplifer::getChartData()
{
    return eegdata.getChartData(32);
}

QList<QList<double>>SelfAmplifer::getRawData()
{
    QList<QList<double>> data;
    data.append(eegdata.getRawData());

    return data;
}
QList<uint8_t> SelfAmplifer::getEEGGameIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getEEGIndex()
{
    return {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
}

QList<uint8_t> SelfAmplifer::getEyeIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getMuscleIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getBreathIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getHeartIndex()
{
    return {};
}

bool SelfAmplifer::connectStatus()
{
    return communication->connectstatus();
}

void SelfAmplifer::setDecodeStatus(bool status)
{
    parse.setDecodeStatus(status);
}
void SelfAmplifer::init()
{
    channel_num=parse.getChannelNum();//初始化通道数
    status=false;//初始化采集器状态
    //初始化TCP通信
    communication=manage.setType(0);
    //初始化通信与解析的连接
    connect(communication,&Communication::readyRead,&parse,&Parse::append);
    connect(communication,&Communication::connected,&parse,&Parse::start);
    connect(communication,&Communication::newLogging,[=](QString logginng){
        qDebug()<<logginng;
    });
    //初始化解析与存储的连接
    connect(&parse,&Parse::parseFinished,this,[=](QList<double> data,QList<double> data_){
        if(status)
        {
            eegdata.append(data,data_);
        }
    });

}
