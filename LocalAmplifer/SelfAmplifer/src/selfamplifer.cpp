#include "selfamplifer.h"
#include "QDebug"

SelfAmplifer::SelfAmplifer()
{
    qRegisterMetaType<QList<double>>("QList<double>");
    init();
}

SelfAmplifer::~SelfAmplifer()
{
    QThread::msleep(20);
    //delete amplifierdata;
    delete matread;
    delete connectwidget;
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
    return {"FP1","FP2" };
}

quint8 SelfAmplifer::getChannnelNum()
{
    return channel_num;
}

quint16 SelfAmplifer::getSampleRate()
{
    return 500;
}

QWidget *SelfAmplifer::getConnectWidget()
{
    return connectwidget;
    //return communication->getConnectWidget();
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
void SelfAmplifer::getLocalData(QString Dir) {
    //QString Dir = "H:/brainheart/T_001_2_fly.mat";

    qDebug() << Dir;
    Raw_data = matread->readlocalmat(Dir);
    Label_data = matread->readlocallabel(Dir);
    connect_status = true;

}
QList<QList<double> > SelfAmplifer::getChartData()
{
    //return eegdata.getChartData(32);
    QList<QList<double>> data;
    if (Raw_data.isEmpty()) {
        qDebug() << "nodata";
        return data;
    }
    //qDebug() << "bias1" << bias;
    int samplenum = Raw_data.first().size();
    //qDebug() << "samplenum"<<samplenum;
    for (int i = getpoint * bias; i < getpoint * bias + getpoint; i++)
    {
        if (samplenum < getpoint * bias + getpoint)
        {
            bias = 0;
            break;
        }

        QList<double> selectchannelData;
        for (const auto& channelData : Raw_data) {
            selectchannelData.append(channelData[i]);
        }
        data.append(selectchannelData);
    }
    //    qDebug() << bias;
    incrementB();
    return data;
}

QList<QList<double>>SelfAmplifer::getRawData()
{
    QList<QList<double>> data;
    if (Raw_data.isEmpty()) {
        qDebug() << "nodata";
        return data;
    }
    //qDebug() << "bias"<< bias;
    int samplenum = Raw_data.first().size();
    //qDebug() << "samplenum"<<samplenum;
    if (bias < samplenum) {
        QList<double> timePointData;
        for (const auto& channel : Raw_data) {
            timePointData.append(channel[bias]);
        }
        data.append(timePointData);
    }
    
    //incrementB();
    return data;
    //QList<QList<double>> data;
    //data.append(eegdata.getRawData());
    //return data;
    //QList<double> data;
    //if (Raw_data.isEmpty()) {
    //    return data;
    //}
    //int channel_num = Raw_data.size();
    //int samplenum = Raw_data.first().size();
    //QList<double> eegchannelData;
    //for (int i = getpoint * bias; i < getpoint * bias + getpoint; i++)
    //{
    //    if (samplenum < getpoint * bias + getpoint)
    //    {
    //        bias = 0;
    //        break;
    //        connect_status = false;
    //    }
    //    for (int j = 0; j < channel_num; j++) {
    //        QList<double> channelData = Raw_data.at(j);
    //        eegchannelData.append(channelData.at(i));
    //    }
    //}
    //data.append(eegchannelData);

    //incrementB();
    //return data;
    //return eegdata.getRawData();
}
QList<uint8_t> SelfAmplifer::getEEGGameIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getEEGIndex()
{
    return {0,1};
}

QList<uint8_t> SelfAmplifer::getEyeIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getMuscleIndex()
{
    QList<uint8_t> data;

    data.append(Label_data);
    return data;
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
    return connect_status;
    //return communication->connectstatus();
}

void SelfAmplifer::setDecodeStatus(bool status)
{
    parse.setDecodeStatus(status);
}
void SelfAmplifer::setAmplifierParam(QString param_field, QVariant param_value)
{
    communication->setParam(param_field, param_value);
}
void SelfAmplifer::initConnetWidget()
{
    connectwidget = new ConnectWidget;
    connect(connectwidget, &ConnectWidget::fileSelected, this, &SelfAmplifer::getLocalData);
}
void SelfAmplifer::init()
{
    matread = new MatRead;
    connect_status = false;
    initConnetWidget();
    channel_num = 2;//初始化通道数
    status = false;//初始化采集器状态
    //channel_num=parse.getChannelNum();//初始化通道数
    //status=false;//初始化采集器状态
    ////初始化TCP通信
    //communication=manage.setType(0);
    ////初始化通信与解析的连接
    //connect(communication,&Communication::readyRead,&parse,&Parse::append);
    //connect(communication,&Communication::connected,&parse,&Parse::start);
    //connect(communication,&Communication::newLogging,[=](QString logginng){
    //    qDebug()<<logginng;
    //});
    ////初始化解析与存储的连接
    //connect(&parse,&Parse::parseFinished,this,[=](QList<double> data,QList<double> data_){
    //    if(status)
    //    {
    //        eegdata.append(data,data_);
    //    }
    //});

}
