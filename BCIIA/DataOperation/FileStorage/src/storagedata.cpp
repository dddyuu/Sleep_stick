#include "storagedata.h"
#include "QDebug"
using namespace File;
Data::Data()
{

}

Data::~Data()
{
	if (!data.isEmpty())
	{
		clear();
	}
}
void Data::appendlabel(quint8 label1) {
    QMutexLocker locker(&labelMutex);  // 加锁
    label.append(label1);
}
void Data::append(QList<QList<double>> data)
{
    // qDebug() << "Data received. Current label count:" << label.size();
	if (data.isEmpty())
	{
		return;
	}
	quint8 type_num = signals_name.size();
	if (type_num != data.size())
	{
        qDebug()<<"type_num"<<type_num;
		qDebug() << data.size();
		qDebug() << "storagedata: size not match";
		throw "storagedata: size not match";
	}
	else
	{
		for (int i = 1; i < type_num; i++)
		{
			memcpy(this->data[i]+data_bias[i], data[i].toVector().data(), sizeof(double) * data[i].size());
			data_bias[i] += data[i].size();
		}
		int eeg_num = data[0].size() / signals_channel_num[0];
		for (int i = 0; i < eeg_num; i++)
		{
			if (data_bias[0] == signals_srate[0] * time_len*signals_channel_num[0])
			{
				QList<double> value = data[0].mid(i * signals_channel_num[0], signals_channel_num[0]);
				data2.append(value);
			}
			else
			{
				memcpy(this->data[0] + data_bias[0], data[0].toVector().data()+i*signals_channel_num[0], sizeof(double) * signals_channel_num[0]);
				data_bias[0]+=signals_channel_num[0];
			}
			
		}
	}
}
void Data::setSignalsName(QStringList name)
{
	signals_name = name;
}

void Data::setSignalsChannelNum(QList<quint8> channel_num)
{
	signals_channel_num = channel_num;
}

void Data::setSignalsSrate(QList<unsigned int> srate)
{
	signals_srate = srate;
}

void Data::setSingals_label(QStringList label)
{
	signals_label = label;
}

void File::Data::setTimeLen(quint64 value)
{
	time_len = value;
}

void Data::alloc_data()
{
	//数据类型数目
	quint8 type_num = signals_name.size();
	//清除之前的数据空间
	clear();
	//自动分配数据空间
	for (int i = 0; i < type_num; i++)
	{
		double* signal_data = (double*)malloc(sizeof(double) * signals_channel_num[i] * signals_srate[i]*(time_len+1));
		this->data.append(signal_data);
		this->data_bias.append(0);
	}

}

void Data::reset()
{
	//重置偏置
	for (int i = 0; i < signals_name.size(); i++)
	{
		data_bias[i] = 0;
	}
	//将二级缓存转入一级缓存
	QList<QList<double>> data;
	data.append(data2);
	if (signals_name.size() != 1)
	{
		data.append(QList<double>());
	}
	append(data);
	data2.clear();
    QMutexLocker locker(&labelMutex);
    label.clear();
}
bool Data::bufferFull()
{
	
	if (data_bias[0] == signals_srate[0] * time_len *signals_channel_num[0])
	{
		return true;
	}
	else
	{
		return false;
	}
}

QList<double*> File::Data::getData()
{
	return this->data;
}
QList<quint8> Data::getlabel() const {
    QMutexLocker locker(&labelMutex);  // 加锁
    return label;
}


QList<quint64> File::Data::getLen()
{
	qDebug() << "signals_srate[0]:" << signals_srate[0];
	return data_bias;
}

QStringList File::Data::getSignalsName()
{
	return signals_name;
}

QList<quint8> File::Data::getSignalsChannelNum()
{
	return signals_channel_num;
}

QList<unsigned int> File::Data::getSignalsSrate()
{
	return signals_srate;
}

QStringList File::Data::getSignalsLabel()
{
	return signals_label;
}

void Data::clear()
{
	for (int i = 0; i < data.size(); i++)
	{
		free(data[i]);
	}
	data.clear();
	data_bias.clear();
}
