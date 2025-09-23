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
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QPainter>
#include <QPixmap>

QT_CHARTS_USE_NAMESPACE

// 时间范围枚举
enum TimeRange {
    OneMinute = 1,
    TwoMinutes = 2,
    FiveMinutes = 5
};

// 数据记录结构体
struct DataRecord {
    QDateTime timestamp;
    qreal predictedValue;  // 预测值
    qreal actualValue;     // 真实值 (本地标签)
    QString predictedLabel; // 预测标签描述
    QString actualLabel;    // 真实标签描述
    bool hasActualValue;    // 是否有真实值

    DataRecord() : hasActualValue(false) {}
    DataRecord(QDateTime time, qreal pred, const QString& predLabel)
        : timestamp(time), predictedValue(pred), predictedLabel(predLabel), hasActualValue(false) {
    }
    DataRecord(QDateTime time, qreal pred, qreal actual, const QString& predLabel, const QString& actLabel)
        : timestamp(time), predictedValue(pred), actualValue(actual),
        predictedLabel(predLabel), actualLabel(actLabel), hasActualValue(true) {
    }
};

class Chart : public QWidget
{
    Q_OBJECT

        QString chartname;
public:
    Chart(QWidget* parent = 0, QString _chartname = "高负荷指数");
    quint8 result;
    uint8_t localresult;
    bool is_receive = 0;
    ~Chart() {}

public slots:
    void connectdata(quint8 data, uint8_t localdata);
    void localLabel(QList<int> label_local);//离线加载
    void saveDataToExcel(); // 保存数据到Excel/CSV
    void saveChartToImage(); // 新增：保存图表为图像
    void saveDataAndChart(); // 新增：同时导出数据和图表

private slots:
    void receiveDatas();
    void updateChart();
    void updatePieChart();  // 更新饼图的槽函数
    void setTimeRange(TimeRange range); // 设置时间范围

private:
    QTcpSocket* socket;
    QLineSeries* series;
    QLineSeries* series_local;
    QTimer* timer;
    QChart* chart1;
    QChartView* chartView;
    QDateTimeAxis* xAxis;

    // 数据存储
    QList<QPair<QDateTime, qreal>> dataPoints; // 存储所有数据点
    QList<QPair<QDateTime, qreal>> localDataPoints;
    QList<DataRecord> comparisonData; // 用于Excel导出的数据记录
    bool isPlottingEnabled = true; // 控制绘图是否启用
    bool flag;
    int dataCount;
    QTimer* updateTimer;
    int CharLenth;
    double data;
    QColor color;//设置点颜色
    TimeRange currentTimeRange; // 当前时间范围

    // 饼图相关成员
    QPieSeries* pieSeries;     // 饼图数据系列
    QChart* pieChart;          // 饼图图表
    QChartView* pieChartView;  // 饼图视图
    int clearCount;            // 低负荷次数计数
    int fatigueCount;          // 高负荷次数计数
    QWidget* pieChartContainer; // 饼图容器
    QLabel* clearPercentageLabel;  // 低负荷百分比标签
    QLabel* fatiguePercentageLabel; // 高负荷百分比标签

    // 私有方法
    void init();
    void initPieChart();       // 初始化饼图的函数
    void updatePercentageLabels(); // 更新百分比标签
    void updateChartFromData();
    QString getExcelFilePath(); // 获取Excel文件路径
    QString getChartImageFilePath(); // 新增：获取图表图像保存路径
    QPixmap createChartImage(); // 新增：创建图表图像
    QString generateStatsText(); // 新增：生成统计信息文本
    bool saveDataToFile(const QString& filePath); // 新增：保存数据到指定文件
    QString valueToLabel(qreal value); // 将数值转换为标签描述
    void addComparisonRecord(QDateTime timestamp, qreal predicted, qreal actual = -1); // 添加对比记录
};

#endif // CHART_H