#ifndef CHART_H
#define CHART_H

#include <QWidget>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLineSeries>
#include <QtCharts>
#include <QDateTime>
#include <QPair>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QMutex>

QT_CHARTS_USE_NAMESPACE

// 时间范围枚举
enum TimeRange {
    OneMinute = 1,
    TwoMinutes = 2,
    FiveMinutes = 5
};
class Chart : public QWidget
{
    Q_OBJECT


    QString chartname;
public:
    Chart(QWidget* parent = 0, QString _chartname = "疲劳指数");
    quint8 result;
    uint8_t localresult;
    bool is_receive=0;
    // QList<uint8_t> localLabelData;
    ~Chart(){}
public slots:
        void connectdata(quint8 data, uint8_t localdata);
        void localLabel(QList<int> label_local);//离线加载
        // void updateLabelData(QList<uint8_t> label_data);
private slots:
    void receiveDatas();
    void updateChart();
    void updatePieChart();  // 更新饼图的槽函数
    void setTimeRange(TimeRange range); // 设置时间范围
private:


    QTcpSocket *socket;
    QLineSeries *series;
    QLineSeries *series_local;
    QTimer *timer;
    QChart *chart1;
    QChartView *chartView;
    QDateTimeAxis *xAxis;
    //QMetaObject::Connection connection;

        // 数据存储
    QList<QPair<QDateTime, qreal>> dataPoints; // 存储所有数据点
    QList<QPair<QDateTime, qreal>> localDataPoints;
    bool isPlottingEnabled = true; // 控制绘图是否启用
    bool flag;
    int dataCount;
    QTimer *updateTimer;
    int CharLenth;
    double data;
    QColor color;//设置点颜色
    TimeRange currentTimeRange; // 当前时间范围

    // 饼图相关新成员
    QPieSeries *pieSeries;     // 饼图数据系列
    QChart *pieChart;          // 饼图图表
    QChartView *pieChartView;  // 饼图视图
    int clearCount;            // 清醒次数计数
    int fatigueCount;          // 疲劳次数计数
    QWidget *pieChartContainer; // 饼图容器 [新增]
    QLabel *clearPercentageLabel;  // 清醒百分比标签
    QLabel *fatiguePercentageLabel; // 疲劳百分比标签
//    int clearCount;            // 清醒次数计数
//    int fatigueCount;          // 疲劳次数计数

    void init();
    void initPieChart();       // 初始化饼图的函数
    void updatePercentageLabels(); // 更新百分比标签
    void updateChartFromData();


};
#endif // CHART_H

