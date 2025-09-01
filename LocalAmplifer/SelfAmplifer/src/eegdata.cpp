#include "eegdata.h"
#include "QDebug"
EEGData::EEGData()
{

}

EEGData::~EEGData()
{
    chart_data.clear();
}
void EEGData::clear()
{
    chart_data.clear();
}
void EEGData::append(QList<double> chart_data,QList<double> raw_data)
{
    mutex.lock();
    this->chart_data.append(chart_data);
    this->raw_data.append(raw_data);
    mutex.unlock();
}

QList<QList<double>> EEGData::getChartData(int channel_num)
{
    QList<QList<double>> data;
    mutex.lock();
    quint16 point_num=this->chart_data.size()/channel_num;
    for(int i=0;i<point_num;i++)
    {
        QList<double> channel_data=this->chart_data.mid(0,channel_num);
        this->chart_data.erase(this->chart_data.begin(),this->chart_data.begin()+channel_num);
        data.append(channel_data);
    }
    mutex.unlock();
    return data;
}

QList<double> EEGData::getRawData()
{

    mutex.lock();
    QList<double> data=this->raw_data;
    raw_data.clear();
    mutex.unlock();
    return data;
}
