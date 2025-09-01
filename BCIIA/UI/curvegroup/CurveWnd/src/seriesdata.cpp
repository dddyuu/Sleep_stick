#include "seriesdata.h"
#include "QDebug"
SeriesData::SeriesData(QObject *parent) : QObject(parent)
{
    this->point_index=0;
    this->max_point_num=100;
    this->scale=1;
    this->raw_points.length=0;
    this->chart_point.length=0;
}
void SeriesData::append(double data)
{
    

    int data_length = chart_point.length;
    if (data_length < max_point_num)
    {
        this->raw_points.x.append(data_length);
        this->raw_points.y.append(data);
        this->raw_points.length++;
        this->chart_point.x.append(data_length);
        this->chart_point.y.append(data * scale + (max_y - min_y) * Id);
        this->chart_point.length++;
        //qDebug() << data * scale + (max_y - min_y) * Id;
        return;
    }
    this->raw_points.y.replace(point_index, data);
    this->chart_point.y.replace(point_index, data * scale + (max_y - min_y) * Id);
    point_index++;
    point_index %= max_point_num;
}
void SeriesData::clear()
{
    raw_points.x.clear();
    raw_points.y.clear();
    raw_points.length=0;
    chart_point.x.clear();
    chart_point.y.clear();
    chart_point.length=0;
}

void SeriesData::setId(int id)
{
    this->Id=id;
}

void SeriesData::setMaxPointNum(int num)
{
    max_point_num=num;
}

void SeriesData::setAxisYMinMax(int min, int max)
{
    min_y=min;
    max_y=max;
}
void SeriesData::increaseScaleFactor()
{
    clear();
    if(scale<=10)
    {
        scale++;
    }

}
void SeriesData::reduceScaleFactor()
{
    clear();
    if(scale>1)
    {
         scale--;
    }
    else
    {
        if(scale>=0.5)
        {
           scale-=0.1;
        }
    }
}

void SeriesData::rescale()
{
    if(raw_points.y.size()==0)
    {
        return;
    }
    int max_value=max(raw_points.y);
    int min_value=min(raw_points.y);

    this->min_value=min_value;
    this->max_value=max_value;

    min_value=abs(min_value);

    max_value=max_value>min_value?max_value:min_value;
    if(max_value<100)
    {
        return;
    }
    for(int i=0;i<raw_points.y.size();i++)
    {
        double orgin_value=raw_points.y[i];
        double new_value=orgin_value/(max_value/100);
        chart_point.y[i]=new_value*scale+(max_y-min_y)*Id;
    }
}

void SeriesData::setXRange(int max_point_num)
{
   clear();
   this->max_point_num=max_point_num;
}
double SeriesData::min()
{
    return min_value;
}

double SeriesData::max()
{
    return max_value;
}

double SeriesData::max(QVector<double> data)
{
    int data_len=data.size();
    if(data_len==1)
    {
        return data[0];
    }
    int max_left=max(data.mid(0,data_len/2));
    int max_right=max(data.mid(data_len/2,data_len));
    return max_left>max_right?max_left:max_right;
}

double SeriesData::min(QVector<double> data)
{
    int data_len=data.size();
    if(data_len==1)
    {
        return data[0];
    }
    int min_left=min(data.mid(0,data_len/2));
    int min_right=min(data.mid(data_len/2,data_len));
    return min_left<min_right?min_left:min_right;
}
