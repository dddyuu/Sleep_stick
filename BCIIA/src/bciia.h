#ifndef BCIIA_H
#define BCIIA_H
#include"dataoperation.h"
#include "bcimonitor.h"

class BCIIA:public QObject
{
	Q_OBJECT
public:
	BCIIA(QObject *parent=nullptr);
	~BCIIA();
	QWidget* getMonitorWidget();
signals:
	void chartDataFinished(QList<double>);
    void locallabelFinished(QList<uint8_t>);
    void sendlabel(quint8 data);
	void rawDataFinished(QList< QList<double>>);
	void preproDatafinished(QList<double>);
	void handleClassificationResult(int classificationResult);
public slots:
    void reciveSubName(QString subname);
private:
	DataOperation* dataoperation;
	void initDataOperation();
	void setDataOperationConnect();
	BCIMonitor* bcimonitor;
	void initBCIMonitor();
	void setBCIMonitorConnect();
    //保存路径
    QString filename;
    void setFileSavePath();


};
#endif // !BCIIA_H
