#include "datathread.h"
#include "QDebug"
#include <QMetaType>

// 在文件顶部声明元类型
Q_DECLARE_METATYPE(QList<uint8_t>)
DataThread::DataThread(QObject *parent) : QThread(parent)
{
    qRegisterMetaType<QList<uint8_t>>("QList<uint8_t>");
    this->destroyFlag=false;
    this->status=false;
}
void DataThread::run()
{
    while (1) {
        if(status&&(device!=NULL))
        {
            for (int i = 0;i < 10;i++)
            {

                QList<uint8_t> label_data = device->getMuscleIndex();
                emit locallabelFinished(label_data);
                QList<QList<double>> raw_data=device->getRawData();
				
                emit rawDataFinished(raw_data);
                QList<QList<double>> all_data=device->getChartData();
                
                int buf_num=all_data.size();

                for (int i = 0;i < buf_num;i++) {
                    QList<double> chart_data = all_data.at(i);
                    // qDebug() << "滤波前:"<<chart_data;
                    emit preproDatafinished(chart_data);
                    chart_data = filter.filterData(chart_data);
                    // qDebug() << "滤波后:" << chart_data;
                    if (!chart_data.isEmpty())
                    {
                        emit chartDataFinish(chart_data);
                        if (destroyFlag)
                        {
                            destroyFlag = false;
                            return;
                        }
                    }
                }
            }
            
        }
        if(destroyFlag)
        {
            destroyFlag=false;
            return;
        }
        msleep(20);
    }
}

void DataThread::setDeivce(AmplifierDevice *device)
{
    this->device=device;
    filter.setChannelNum(device->getChannnelNum());

    filter.setEEGIndex(device->getEEGIndex()); //EEG全部通道索引
    filter.setMuscleIndex(device->getMuscleIndex()); //肌电通道索引
    filter.setBreathIndex(device->getBreathIndex()); //呼吸通道索引
    filter.setHeartIndex(device->getHeartIndex()); //心电通道索引
    filter.setSampleRate(device->getSampleRate());
//    qDebug()<<device->getEEGIndex();
//    qDebug()<<device->getHeartIndex();
}
void DataThread::setStatus(bool value)
{
    status = value;
}

void DataThread::destroy()
{
    destroyFlag = true;
}

bool DataThread::getStatus() const
{
    return status;
}

void DataThread::clearFilter()
{
    filter.init();
}
