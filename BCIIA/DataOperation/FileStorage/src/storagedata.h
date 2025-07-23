#ifndef STORAGEDATA_H
#define STORAGEDATA_H
#include "vector"
#include "QString"
#include "QList"
#include <QMutex>
#include <QMutexLocker>
namespace File {
	class Data;
}
class File::Data
{
public:
	Data();
	~Data();
	void append(QList<QList<double>>);
    void appendlabel(quint8 label);
	void setSignalsName(QStringList);
	void setSignalsChannelNum(QList<quint8>);
	void setSignalsSrate(QList<unsigned int>);
	void setSingals_label(QStringList);
	void setTimeLen(quint64);
	void alloc_data();
	//重置数据的偏移量
	void reset();
	//缓冲区是否已满
	bool bufferFull();

	//获取
	QList<double*> getData();
	QList<quint64> getLen();
    QList<quint8> getlabel() const;  // 修改为const
	QStringList getSignalsName();
	QList<quint8> getSignalsChannelNum();
	QList<unsigned int> getSignalsSrate();
	QStringList getSignalsLabel();
private:

	//数据存储类型
	QList<double*> data;
	//数据二级缓存
	QList<double>  data2;
	//数据的偏移量
	QList<quint64> data_bias;
	void clear();
	//信号类别
	QStringList signals_name;
	//信号的通道数
	QList<quint8> signals_channel_num;
	//信号采样率
	QList<unsigned int> signals_srate;
	//信号的标签
	QStringList signals_label;
	//缓存区的数据时间长度
	quint64 time_len;
    // 添加互斥锁保护标签
    mutable QMutex labelMutex;  // 修改点1：添加互斥锁
    QList<quint8>label;

};
#endif // !STORAGEDATA_H

