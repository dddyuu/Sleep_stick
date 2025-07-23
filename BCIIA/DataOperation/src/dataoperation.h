#ifndef DATAOPERATION_H
#define DATAOPERATION_H
#include <QObject>
#include "filestorage.h"
#include "amplifier.h"
#include "datainfo.h"
class DataOperation:public QObject
{
	Q_OBJECT
public:
	explicit DataOperation(QObject *parent=nullptr);
	~DataOperation();
	DataInfo getDataInfo();
	//---------采集功能---------//
	//开始监听
	void startMonitor();
	//停止监听
	void stopMonitor();
	//获取插件管理软件
	QWidget* getPluginWidget();
	//打开插件窗口
	void showPluginWidget();
	//获取放大器连接窗口
	QWidget* getConnectWidget();
	//打开连接窗口
	void showConnectWidget();
	//---------存储功能---------//
	//设置文件保存位置
	void setFileName(QString);
	//开始保存
	void startSave();
	//停止保存
	void stopSave(); 
	//暂停保存
	void pauseSave();

	//解密
	void decode(bool);
signals:
	void loadPluginSucceed();
	void chartDataFinished(QList<double>);
	void connected();
    void locallabelFinished(QList<uint8_t>);
private:
	Amplifier* amplifier;
	FileStorage* fileStorage;
	DataInfo datainfo;
    // BCIIA bciia;
	void readDataInfo();
    QString filename;

};
#endif // !DataOperation


