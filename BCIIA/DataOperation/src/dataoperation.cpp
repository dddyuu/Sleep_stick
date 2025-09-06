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
	connect(amplifier, &Amplifier::preproDatafinished, this, &DataOperation::preproDatafinished);

	connect(amplifier, &Amplifier::rawDataFinished,fileStorage, &FileStorage::append);

	connect(amplifier, &Amplifier::loadPluginSucceed, this, &DataOperation::readDataInfo);
	connect(amplifier, &Amplifier::loadPluginSucceed, this, &DataOperation::loadPluginSucceed);
	connect(amplifier, &Amplifier::chartDataFinished, this, &DataOperation::chartDataFinished);
	connect(amplifier, &Amplifier::connected, this, &DataOperation::connected);

    connect(amplifier, &Amplifier::locallabelFinished,this, &DataOperation::locallabelFinished);
	// 连接Python整数接收信号到处理槽函数
	connect(fileStorage, &FileStorage::pythonIntReceived, this, &DataOperation::handlePythonIntData);

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
// 处理从Python接收到的分类结果
void DataOperation::handlePythonIntData(int value)
{
	qDebug() << "DataOperation received Python classification result:" << value;

	// 验证接收到的分类结果是否有效
	if (value >= 0 && value <= 2) {
		// 将分类结果通过信号传输出去
		emit pythonClassificationReceived(value);
	}
}