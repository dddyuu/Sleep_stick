#include "filter.h"
#include "qdebug.h"

Filter::Filter()
{
    iirfilter=new IIRFILTER();
}

void Filter::init()
{
    iirfilter->initW();
}

QList<double> Filter::filterData(QList<double> data)
{

//    for(int i=0;i<channel_num;i++)
//    {
//        data[i]=iirfilter->B_filter(channel_num,i,data[i]);
//        //高通滤波
//        data[i]=iirfilter->iir(high_a,high_b,&iirfilter->matrixh[i][0][0],data[i],2);
//        //低通滤波
//        data[i]=iirfilter->iir(low_a,low_b,&iirfilter->matrixl[i][0][0],data[i],2);

//    }
    //工频滤波
    // qDebug()<<"channel_num"<<channel_num;
    for(int i=0;i<channel_num;i++)
    {
         data[i]=iirfilter->B_filter(channel_num,i,data[i]);
    }

    for(uint8_t i: EEGIndex)
    {
//        data[i]=iirfilter->B_filter(channel_num,i,data[i]);
        //高通滤波
        // data[i]=iirfilter->iir(high_a,high_b,&iirfilter->matrixh[i][0][0],data[i],2);
        //低通滤波
        data[i]=iirfilter->iir(low_a,low_b,&iirfilter->matrixl[i][0][0],data[i],2);
    }

//    for(uint8_t j: muscleIndex)
//    {
//        data[j]=iirfilter->B_filter(channel_num,j,data[j]);
//    }
//    for(uint8_t k: breathIndex)
//    {
//        data[k]=iirfilter->B_filter(channel_num,k,data[k]);
//    }
//    for(uint8_t l: heartIndex)
//    {
//        data[l]=iirfilter->B_filter(channel_num,l,data[l]);
//    }
    return data;
}

void Filter::setChannelNum(uint8_t num)
{
    channel_num=num;
}

void Filter::setSampleRate(uint16_t srate)
{
    if(srate==250)
    {
        low_b=&iirfilter->low_b250[0][0];
        low_a=&iirfilter->low_a250[0][0];
        high_b=&iirfilter->high_b250[0][0];
        high_a=&iirfilter->high_a250[0][0];
    }
    else if(srate==500)
    {
		qDebug() << "srate==500";
        low_b=&iirfilter->low_b500[0][0];
        low_a=&iirfilter->low_a500[0][0];
        high_b=&iirfilter->high_b500[0][0];
        high_a=&iirfilter->high_a500[0][0];
    }
    else if(srate==1000)
    {
        printf("1000");
        qDebug() << "srate==1000";
        low_b=&iirfilter->low_b1000[0][0];
        low_a=&iirfilter->low_a1000[0][0];
        high_b=&iirfilter->high_b1000[0][0];
        high_a=&iirfilter->high_a1000[0][0];
    }
    else if (srate == 2000) {
        printf("2000");
        qDebug() << "srate==2000";
        low_b = &iirfilter->low_b2000[0][0];
        low_a = &iirfilter->low_a2000[0][0];
        high_b = &iirfilter->high_b2000[0][0];
        high_a = &iirfilter->high_a2000[0][0];
    }
}

void Filter::setMuscleIndex(const QList<uint8_t> &value)
{
    muscleIndex = value;
}

void Filter::setBreathIndex(const QList<uint8_t> &value)
{
    breathIndex = value;
}

void Filter::setHeartIndex(const QList<uint8_t> &value)
{
    heartIndex = value;
}

void Filter::setEEGIndex(const QList<uint8_t> &value)
{
    EEGIndex = value;
}

void Filter::setEEGGameIndex(const QList<uint8_t> &value)
{
    EEGGameIndex = value;
}



