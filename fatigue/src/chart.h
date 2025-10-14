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
    qreal predictedValue;
    qreal actualValue;
    QString predictedLabel;
    QString actualLabel;
    bool hasActualValue;

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
    void localLabel(QList<int> label_local);
    void saveDataToExcel();
    void saveChartToImage();
    void saveDataAndChart();

private slots:
    void receiveDatas();
    void updateChart();
    void updatePieChart();
    void setTimeRange(TimeRange range);

private:
    QTcpSocket* socket;
    QLineSeries* series;
    QLineSeries* series_local;
    QTimer* timer;
    QChart* chart1;
    QChartView* chartView;
    QValueAxis* xAxis;                 // 改为数值轴

    // 数据存储（仍保留时间，仅用于统计/导出）
    QList<QPair<QDateTime, qreal>> dataPoints;
    QList<QPair<QDateTime, qreal>> localDataPoints;
    QList<DataRecord> comparisonData;
    bool isPlottingEnabled = true;
    bool flag;
    int dataCount;
    QTimer* updateTimer;
    int CharLenth;
    double data;
    QColor color;
    TimeRange currentTimeRange;

    // 索引轴相关
    int MaxPoints = 360;               // 显式初始化为 360

    // 饼图相关成员
    QPieSeries* pieSeries;
    QChart* pieChart;
    QChartView* pieChartView;
    int clearCount;
    int fatigueCount;
    QWidget* pieChartContainer;
    QLabel* clearPercentageLabel;
    QLabel* mediumPercentageLabel;
    QLabel* fatiguePercentageLabel;

    // 私有方法
    void init();
    void initPieChart();
    void updatePercentageLabels();
    void updateChartFromData();
    QString getExcelFilePath();
    QString getChartImageFilePath();
    QPixmap createChartImage();
    QString generateStatsText();
    bool saveDataToFile(const QString& filePath);
    QString valueToLabel(qreal value);
    void addComparisonRecord(QDateTime timestamp, qreal predicted, qreal actual = -1);

    // 辅助：根据当前数据量设置 X 轴范围与刻度
    void adjustXAxisForCount(int count);
};

#endif // CHART_H