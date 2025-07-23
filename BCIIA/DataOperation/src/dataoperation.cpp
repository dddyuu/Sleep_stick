#include "dataoperation.h"
#include "dataoperation.h"
#include "dataoperation.h"
#include "dataoperation.h"
#include "dataoperation.h"
#include "DataOperation.h"
DataOperation::DataOperation(QObject* parent):QObject(parent)
{
	amplifier = new Amplifier;
    // fileStorage = new FileStorage;
    // 正确的方式（使用单例）
    fileStorage = FileStorage::instance(parent);

	//信号连接

	connect(amplifier, &Amplifier::rawDataFinished,fileStorage, &FileStorage::append);

	connect(amplifier, &Amplifier::loadPluginSucceed, this, &DataOperation::readDataInfo);
	connect(amplifier, &Amplifier::loadPluginSucceed, this, &DataOperation::loadPluginSucceed);
	connect(amplifier, &Amplifier::chartDataFinished, this, &DataOperation::chartDataFinished);
	connect(amplifier, &Amplifier::connected, this, &DataOperation::connected);

    connect(amplifier, &Amplifier::locallabelFinished,this, &DataOperation::locallabelFinished);
}

DataOperation::~DataOperation()
{
	delete amplifier;
	delete fileStorage;
}
DataInfo DataOperation::getDataInfo()
{
	return this->datainfo;
}
void DataOperation::startMonitor()
{
	amplifier->start();
}
void DataOperation::stopMonitor()
{
	amplifier->stop();
}
QWidget* DataOperation::getPluginWidget()
{
	return amplifier->getPluginWidget();
}
void DataOperation::showPluginWidget()
{
	amplifier->getPluginWidget()->show();
}
QWidget* DataOperation::getConnectWidget()
{
	return amplifier->getConnectWidget();
}
void DataOperation::showConnectWidget()
{
	if (amplifier->getConnectWidget()!=NULL)
	{
		amplifier->getConnectWidget()->show();
	}
	else
	{
		qDebug() << "加载插件";
	}
}

void DataOperation::setFileName(QString name)
{
    fileStorage->setFileName(name);
}
void DataOperation::startSave()
{
	fileStorage->start();
}

void DataOperation::stopSave()
{
	fileStorage->stop();
}

void DataOperation::pauseSave()
{
	fileStorage->pause();
}
void DataOperation::decode(bool status)
{
	amplifier->setDecodeStatus(status);
}
void DataOperation::readDataInfo()
{
	datainfo.labels = amplifier->getChannelName();
	datainfo.num = amplifier->getChannelName().size();
	datainfo.srate = amplifier->getSampleRate();
	fileStorage->setChanlocs(amplifier->getChanlocs());
	fileStorage->setSrate(datainfo.srate);
}
