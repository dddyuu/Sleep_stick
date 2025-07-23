#include "maxwidget.h"

Maxwidget::Maxwidget(QWidget *parent):QWidget(parent)
{
    init();
}

Maxwidget::~Maxwidget()
{
    delete  chart;
    delete  curveproperty;
    delete  widget_layout;
}

void Maxwidget::setText(QString text)
{
    this->m_title->setText(text);
    this->curveproperty->setTitle(text);
}

void Maxwidget::setRangeX(int start, int end)
{
    this->axis_x->setRange(start,end);
}

void Maxwidget::setRangeY(int start, int end)
{
    this->axis_y->setRange(start,end);
}

void Maxwidget::clear()
{
    this->series->data().clear();
}

int Maxwidget::point_size()
{
    return this->series->data()->size();
}


void Maxwidget::setPoint(double x,double y)
{
    this->series->addData(x,y);
}

void Maxwidget::setPoints(QVector<double> x,QVector<double> y)
{
    this->series->setData(x,y);
    if(!y.isEmpty())
    {
        double max_value=max(y);
        double min_value=min(y);
        double chart_range=(max_value-min_value)/2;
        axis_y->setRange(min_value-chart_range,max_value+chart_range);
        chart->replot();
    }

}

void Maxwidget::setPoints(QVector<double> x, QVector<double> y, double max, double min)
{
    this->series->setData(x,y);
    if(!y.isEmpty())
    {
        double max_value=max;
        double min_value=min;
        double chart_range=(max_value-min_value)/2;
        axis_y->setRange(min_value-chart_range,max_value+chart_range);
        chart->replot();
    }
}

void Maxwidget::init()
{
    this->resize(1000,600);
    this->setWindowTitle("放大窗口");
    chart=new QCustomPlot();
    axis_x=chart->xAxis;
    axis_y=chart->yAxis;
    series=chart->addGraph(axis_x,axis_y);

    axis_x->setRange(0,1000);
    axis_y->setRange(-100,100);
    //标题
    m_title = new QCPTextElement(chart);
    chart->plotLayout()->insertRow(0);
    chart->plotLayout()->addElement(0, 0, m_title);

    this->curveproperty=new CurveProperty();

    this->setStyleSheet("");
    initLayaout();
    setConnect();
}

void Maxwidget::initLayaout()
{
    this->widget_layout=new QHBoxLayout();
    widget_layout->setMargin(0);
    this->widget_layout->addWidget(chart,7);
    this->widget_layout->addWidget(curveproperty,3);
    this->setLayout(widget_layout);
}

void Maxwidget::setConnect()
{
    connect(curveproperty,&CurveProperty::backgroudValueChange,this,&Maxwidget::backgroudValueChange);
    connect(curveproperty,&CurveProperty::axiasXValueChange,this,&Maxwidget::axiasXValueChange);
    connect(curveproperty,&CurveProperty::axiasYValueChange,this,&Maxwidget::axiasYValueChange);
    connect(curveproperty,&CurveProperty::fontValueChange,this,&Maxwidget::fontValueChange);
    connect(curveproperty,&CurveProperty::curveColorValueChange,this,&Maxwidget::curveColorValueChange);
    connect(curveproperty,&CurveProperty::curveWidthValueChange,this,&Maxwidget::curveWidthValueChange);
    connect(curveproperty,&CurveProperty::axisYRangeChange,this,&Maxwidget::axisYRangeChange);
}
void Maxwidget::closeEvent(QCloseEvent *event)
{
    emit closed();
}
void Maxwidget::backgroudValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="背景颜色")
    {
       this->chart->setBackground(value.value<QColor>());
    }
}

void Maxwidget::axiasXValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="x坐标轴颜色")
    {
        QPen pen(value.value<QColor>());
        axis_y->setBasePen(pen);
        chart->replot();
    }

}

void Maxwidget::axiasYValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="y坐标轴颜色")
    {
        QPen pen(value.value<QColor>());
        axis_y->setBasePen(pen);
        chart->replot();
    }
}

void Maxwidget::fontValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="字体颜色")
    {
        this->axis_x->setTickLabelColor(value.value<QColor>());
        this->axis_y->setTickLabelColor(value.value<QColor>());
        chart->replot();
    }
}

void Maxwidget::curveColorValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="曲线颜色")
    {
        this->pen.setColor(value.value<QColor>());
        series->setPen(pen);
    }
}

void Maxwidget::curveWidthValueChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="曲线宽度")
    {
        this->pen.setWidth(value.value<int>());
        series->setPen(pen);
    }
}

void Maxwidget::axisYRangeChange(QtProperty *property, const QVariant &value)
{
    if(property->propertyName()=="纵坐标")
    {
        if(value.toSize().width()>0)
        {
            this->axis_y->setRange(-value.toSize().width(),value.toSize().height());
        }
        else
        {
            this->axis_y->setRange(value.toSize().width(),value.toSize().height());
        }
    }


}

double Maxwidget::max(QVector<double> data)
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

double Maxwidget::min(QVector<double> data)
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
