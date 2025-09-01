#ifndef CURVEGROUP_H
#define CURVEGROUP_H
#include "QWidget"
#include "curvewnd.h"
#include "seriespoint.h"
#include "QToolButton"
#include "datadown.h"
#include "curveconfigwidget.h"
class CurveGroup:public QWidget
{
    Q_OBJECT
public:
    explicit CurveGroup(QWidget *parent=nullptr);
    ~CurveGroup();
    //设置每组显示的曲线数目
    void setCurveNumGroup(int);
    //设置显示的曲线数目
    void setCurveNum(int num);
    //设置y的范围
    void setMinMax(int,int);
    //设置显示的数目
    void setMaxPoint(int);
    //设置标签
    void setCurveLabels(QStringList);
    //设置mark
    void appendMark(QString);

    void setSrate(int);

    //模拟数据
    void timerEvent(QTimerEvent *event);
    //修改y的范围
    void setScale(int value);
    void increaseScale();
    void reduceScale();
    //修改x的范围
    void increaseXRange();
    void reduceXRange();
    //修改通道数
    void reduceGroupChannel();
    void increaseGroupChannel();
    //测试数据
    void test_chart();

    // curvewnd访问器
    CurveWnd *getCurveWnd();

    //
    void setCurveStyle();
    void clear();

    CurveConfigWidget *getCurveconfigwidget() const;

public slots:
    void next();
    void last();
    void append(QList<double>);
signals:
    void first_group();
    void end_group();
    void CurrentData(QList<double>);
private:
    CurveWnd *curvewnd;
    QHBoxLayout *widget_layout;
    int curve_num_group;
    int curve_num;
    int group_num;
    double scale;
    int max_point;
    QStringList labels;
    int current_group;
    void init();
    void setConnect();
    void initLayout();

    //配置窗口
    CurveConfigWidget *curveconfigwidget;
    void initCurveConfigWidget();
};

#endif // CURVEGROUP_H
