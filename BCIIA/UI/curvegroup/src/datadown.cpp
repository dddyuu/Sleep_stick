#include "datadown.h"
#include "QDebug"
DataDown::DataDown(QObject *parent) : QThread(parent)
{
    status=true;
}

DataDown::~DataDown()
{
    status=false;
    msleep(50);
}

void DataDown::setChannelNum(quint8 num)
{
    data.clear();
    channel_num=num;
}

void DataDown::setSrate(const quint16 &value)
{
    this->downsample=value/250;
    start();
}

void DataDown::run()
{
    while (status) {
        mutex.lock();
        if(downsample>1&&downsample<data.size())
        {
            quint16 down_data_num=this->data.size()/downsample;
            for(int k=0;k<down_data_num;k++)
            {
                QList<double> data,value;
                for(int i=0;i<channel_num;i++)
                {
                     double channel_data=0;

                     for(int j=0;j<downsample;j++)
                     {
                        value=this->data.at(j+downsample*k);
                        if(value.size()!=channel_num)
                        {
                            break;
                        }
                        channel_data+=value.at(i);
                     }
                     if(value.size()!=channel_num)
                     {
                         break;
                     }
                     channel_data/=downsample;
                     data.append(channel_data);
                }
                if(value.size()!=channel_num)
                {
                    break;
                }
                emit downData(data);
            }
            this->data.remove(0,downsample*down_data_num);


        }
        else
        {
            while (!data.isEmpty()) {
                QList<double> value=data.takeAt(0);
                emit downData(value);
                usleep(1);
            }
        }
        mutex.unlock();
        msleep(4);
    }
}

void DataDown::receiveData(QList<double> value)
{
    mutex.lock();
    data.append(value);
    mutex.unlock();
}
