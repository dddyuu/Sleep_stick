#include "bciia.h"
#include "datainfo.h"
BCIIA::BCIIA(QObject* parent):QObject(parent)
{
	initDataOperation();
	initBCIMonitor();
	setDataOperationConnect();
	setBCIMonitorConnect();
	connect(dataoperation, &DataOperation::chartDataFinished, this, &BCIIA::chartDataFinished);
	connect(dataoperation, &DataOperation::preproDatafinished, this, &BCIIA::preproDatafinished);
    connect(dataoperation, &DataOperation::locallabelFinished, this, &BCIIA::locallabelFinished);
	connect(dataoperation, &DataOperation::pythonClassificationReceived, this, &BCIIA::handleClassificationResult);
}
BCIIA::~BCIIA()
{
	delete dataoperation;
	delete bcimonitor;
}

QWidget* BCIIA::getMonitorWidget()
{
	return bcimonitor;
}
//命名，收集到的被试名字，并保存
void BCIIA::reciveSubName(QString subname){
    filename = subname;
    dataoperation->setFileName(filename);
    filename = "";
}
void BCIIA::initDataOperation()
{
	dataoperation = new DataOperation;
}

void BCIIA::setDataOperationConnect()
{
	connect(dataoperation, &DataOperation::loadPluginSucceed, this, [=]() {
			DataInfo datainfo=dataoperation->getDataInfo();
			bcimonitor->setCurveLabels(datainfo.labels);
		});
	connect(dataoperation, &DataOperation::chartDataFinished, bcimonitor, &BCIMonitor::append);

	connect(dataoperation, &DataOperation::connected, this, [=]() {
		    qDebug() << "设备开始连接";
			bcimonitor->monitorEnable(true);
		});
}
void BCIIA::initBCIMonitor()
{
	bcimonitor = new BCIMonitor;
}
void BCIIA::setBCIMonitorConnect()
{
	connect(bcimonitor, &BCIMonitor::startMonitor, dataoperation, &DataOperation::startMonitor);
	connect(bcimonitor, &BCIMonitor::stopMonitor, dataoperation, &DataOperation::stopMonitor);
	connect(bcimonitor, &BCIMonitor::openConnect, dataoperation, &DataOperation::showConnectWidget);
	connect(bcimonitor, &BCIMonitor::openPulgin, dataoperation, &DataOperation::showPluginWidget);
    //保存
	connect(bcimonitor, &BCIMonitor::startSave, dataoperation, &DataOperation::startSave);
	connect(bcimonitor, &BCIMonitor::pauseSave, dataoperation, &DataOperation::pauseSave);
	connect(bcimonitor, &BCIMonitor::stopSave, dataoperation, &DataOperation::stopSave);

	//解密
	connect(bcimonitor, &BCIMonitor::decodeSignal, this, [=]() {
		dataoperation->decode(true);
		});
	connect(bcimonitor, &BCIMonitor::encodeSignal, this, [=]() {
		dataoperation->decode(false);
		});
}
