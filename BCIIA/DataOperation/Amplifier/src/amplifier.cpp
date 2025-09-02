#include "amplifier.h"
#include "QDebug"
#include "QWidget"
Amplifier::Amplifier()
{
    initTimer();
    initDataThread();
    initPluginManage();
    initPluginWidget();
}

Amplifier::~Amplifier()
{
    if(amplifierdevice!=NULL)
    {
        amplifierdevice->stop();
    }
    datathread->destroy();
    QThread::msleep(10);
    delete amplifierdevice;
    delete datathread;
    delete timer;
}
void Amplifier::start()
{
    status=true;
    amplifierdevice->start();
    datathread->setStatus(true);
}
void Amplifier::manageBuffer(const QList<QList<double>>& data)
{
    dataBuffer.append(data);
    if (dataBuffer.size() > bufferSize)
    {
        dataBuffer.removeFirst();
    }
}
void Amplifier::pause()
{
    status=false;
    stop();
}

void Amplifier::stop()
{
    status=false;
    amplifierdevice->stop();
    datathread->setStatus(false);
}

QStringList Amplifier::getChannelName()
{
    return amplifierdevice->getChannelName();
}

QVariantList Amplifier::getChanlocs()
{
    QVariantList chanlocs;
    QStringList label=amplifierdevice->getChannelName();
    for(int i=0;i<label.size();i++)
    {
        QMap<QString,QVariant> map;
        map["labels"]=label[i];
        chanlocs.append(map);
    }
    return chanlocs;
}

QWidget *Amplifier::getConnectWidget()
{
    if(amplifierdevice==NULL)
    {
        return  NULL;
    }
    return amplifierdevice->getConnectWidget();
}

QWidget *Amplifier::getConfigWidget()
{
    return amplifierdevice->getConfigWidget();
}
quint16 Amplifier::getSampleRate()
{
    return amplifierdevice->getSampleRate();
}
QList<uint8_t> Amplifier::getEEGIndex()
{ 
    return amplifierdevice->getEEGIndex();
}

QList<uint8_t> Amplifier::getMuscleIndex()
{ 
    return amplifierdevice->getMuscleIndex();
}

QList<uint8_t> Amplifier::getBreathIndex()
{
    return amplifierdevice->getBreathIndex();
}

QList<uint8_t> Amplifier::getHeartIndex()
{
    return amplifierdevice->getHeartIndex();
}


void Amplifier::showConnectWidget()
{
    amplifierdevice->getConnectWidget()->show();
}

void Amplifier::checkConnectStatus()
{
    if(amplifierdevice!=NULL)
    {
        bool status = amplifierdevice->connectStatus();
        if(!status)
        {
            if(datathread->isRunning())
            {
                datathread->setStatus(false);
                datathread->destroy();
                connect_status=false;
                emit this->disconnected();
            }

        }
        else
        {
            if((!connect_status)&&(!datathread->getStatus()))
            {
                connect_status=true;
                datathread->start();
                emit connected();
            }

        }
    }
}
void Amplifier::initTimer()
{
    timer=new QTimer;
    timer->start(1000);
    connect(timer,&QTimer::timeout,this,&Amplifier::checkConnectStatus);
}

void Amplifier::initDataThread()
{
    datathread=new DataThread;
    connect(datathread,&DataThread::chartDataFinish,this,&Amplifier::chartDataFinished);
    connect(datathread,&DataThread::preproDatafinished,this,&Amplifier::preproDatafinished);
    connect(datathread, &DataThread::rawDataFinished, this, &Amplifier::rawDataFinished);
    connect(datathread,&DataThread::locallabelFinished,this,&Amplifier::locallabelFinished);
}

void Amplifier::initPluginManage()
{
    pluginmanage=new PluginManage(&amplifierdevice);
}

PluginWidget *Amplifier::getPluginWidget() const
{
    return pluginwidget;
}

void Amplifier::showPluginWidget()
{
    pluginwidget->show();
}

void Amplifier::setDecodeStatus(bool status)
{
    amplifierdevice->setDecodeStatus(status);
}

void Amplifier::clearFilter()
{
    datathread->clearFilter();
}

void Amplifier::loadPluginStatus(bool status)
{
    if(status)
    {
        qDebug()<<"加载失败";
    }
    else
    {
        qDebug()<<"加载成功";
        emit loadPluginSucceed();
        datathread->setDeivce(amplifierdevice);
    }
}

void Amplifier::initPluginWidget()
{
    pluginwidget=new PluginWidget;
    connect(pluginmanage,&PluginManage::pluginInfoChanged,pluginwidget,&PluginWidget::loadPluginInfo);
    connect(pluginwidget,&PluginWidget::append,pluginmanage,&PluginManage::append);
    connect(pluginwidget,&PluginWidget::remove,pluginmanage,&PluginManage::remove);
    connect(pluginwidget,&PluginWidget::loadPlugin,pluginmanage,&PluginManage::loadPlugin);
    connect(pluginmanage,&PluginManage::error,this,&Amplifier::loadPluginStatus);
    pluginmanage->loadPluginInfo();
}

bool Amplifier::getStatus() const
{
    return status;
}
