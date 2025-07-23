#ifndef DATAINFO_H
#define DATAINFO_H
#include "QObject"
class DataInfo
{
public:
	DataInfo();
	~DataInfo();
	QStringList labels;
	quint8 num;
	quint16 srate;
};
#endif // !DATAINFO_H
