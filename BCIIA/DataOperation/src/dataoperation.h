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
	//---------�ɼ�����---------//
	//��ʼ����
	void startMonitor();
	//ֹͣ����
	void stopMonitor();
	//��ȡ�����������
	QWidget* getPluginWidget();
	//�򿪲������
	void showPluginWidget();
	//��ȡ�Ŵ������Ӵ���
	QWidget* getConnectWidget();
	//�����Ӵ���
	void showConnectWidget();
	//---------�洢����---------//
	//�����ļ�����λ��
	void setFileName(QString);
	//��ʼ����
	void startSave();
	//ֹͣ����
	void stopSave(); 
	//��ͣ����
	void pauseSave();

	//����
	void decode(bool);
public slots:
	// 处理从Python接收到的整数数据的槽函数
	void handlePythonIntData(int value);
signals:
	void loadPluginSucceed();
	void chartDataFinished(QList<double>);
	void connected();
    void locallabelFinished(QList<uint8_t>);
	void preproDatafinished(QList<double>);
	void rawDataFinished(QList<QList<double>>);
	// 将Python分类结果传输出去
	void pythonClassificationReceived(int classificationResult);

private:
	Amplifier* amplifier;
	FileStorage* fileStorage;
	DataInfo datainfo;
    // BCIIA bciia;
	void readDataInfo();
    QString filename;

};
#endif // !DataOperation


