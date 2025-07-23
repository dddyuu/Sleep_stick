
#include "chart.h"
#include "QDebug"


Chart::Chart(QWidget* parent, QString _chartname) : QWidget(parent) {
    setParent(parent);
    chartname = _chartname;
    dataCount = 0;
    CharLenth = 15;
    flag = true;

    // 初始化时间范围
    currentTimeRange = FiveMinutes; // 默认5分钟

    // 初始化计数器
    clearCount = 0;
    fatigueCount = 0;
    // 初始化百分比标签
    clearPercentageLabel = nullptr;
    fatiguePercentageLabel = nullptr;
    pieChartContainer = nullptr;
    // 初始化图表指针
    chart1 = nullptr;
    chartView = nullptr;
    series = nullptr;
    xAxis = nullptr;
    pieChart = nullptr;
    pieChartView = nullptr;
    pieSeries = nullptr;
    series_local = nullptr; // 本地数据曲线
    // localLabelData = QList<uint8_t>();
    // init();
    // QPushButton* testBtn = new QPushButton("TEST", this);
    // testBtn->setGeometry(10, 10, 100, 30); // 绝对定位
    // // 确保按钮启用
    // testBtn->setEnabled(true);

    // 确保按钮可见
    // testBtn->setVisible(true);
    // connect(testBtn, &QPushButton::clicked, []() {
    //     qDebug() << "测试按钮被点击!";
    //     });


}

void Chart::init() {
    QHBoxLayout* mainLayout = new QHBoxLayout();// 创建主水平布局

    chart1 = new QChart();
    chart1->setTitle("疲劳状态监测");
    //初始化线条
    series = new QLineSeries;
    series->setName("实时数据");
    QPen pen1(Qt::blue);
    pen1.setWidth(2);
    series->setPen(pen1);
    chart1->addSeries(series);
    // 初始化本地数据线条
    series_local = new QLineSeries;
    series_local->setName("本地标签");
    QPen pen2(Qt::red);
    pen2.setWidth(2);
    pen2.setStyle(Qt::DashLine); // 虚线
    series_local->setPen(pen2);
    chart1->addSeries(series_local);
    //初始化图
    chartView = new QChartView(chart1);
    chartView->setRenderHint(QPainter::Antialiasing);
    //横坐标
    xAxis = new QDateTimeAxis;
    xAxis->setTickCount(CharLenth); // 设置刻度数量
    xAxis->setFormat("hh:mm:ss"); // 设置时间格
    chart1->addAxis(xAxis, Qt::AlignBottom);
    series->attachAxis(xAxis);
    series_local->attachAxis(xAxis); // 附加本地数据到X轴

    //纵坐标
    QCategoryAxis* axisY = new QCategoryAxis;
    axisY->setMin(0);
    axisY->setMax(100);
    axisY->setStartValue(0);
    axisY->append("清醒", 50);
    axisY->append("疲劳", 100);
    chart1->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    series_local->attachAxis(axisY);

    // 添加图例
    chart1->legend()->setVisible(true);
    chart1->legend()->setAlignment(Qt::AlignBottom);
    // 设置初始横坐标范围
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime futureTime = currentTime.addSecs(CharLenth * 8); // 未来CharLenth个时间节点
    xAxis->setRange(currentTime, futureTime);

    // 初始化饼图部分（右侧）
    initPieChart();

    // 创建时间范围选择按钮
    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(5);

    // 添加标题标签
    QLabel* timeRangeLabel = new QLabel("时间范围选择");
    timeRangeLabel->setAlignment(Qt::AlignLeft);
    QFont labelFont = timeRangeLabel->font();
    labelFont.setPointSize(12);
    labelFont.setBold(true);
    timeRangeLabel->setFont(labelFont);
    timeRangeLabel->setStyleSheet("color: white; background: transparent;");

    QPushButton* btn1min = new QPushButton("1分钟");
    QPushButton* btn2min = new QPushButton("2分钟");
    QPushButton* btn5min = new QPushButton("5分钟");

    // 添加停止绘制按钮
    QPushButton* btnStopPlot = new QPushButton("停止绘制");
    btnStopPlot->setObjectName("stopPlotButton");

    // 设置按钮样式 - 增加悬停效果和按下效果
    QString buttonStyle = "QPushButton {"
        "background-color: #3498db;"
        "color: white;"
        "border: none;"
        "padding: 8px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "background-color: #1c6ea4;"
        "}";

    // 停止按钮特殊样式
    QString stopButtonStyle = "QPushButton#stopPlotButton {"
                              "background-color: #e74c3c;" // 红色表示停止状态
                              "}"
                              "QPushButton#stopPlotButton:hover {"
                              "background-color: #c0392b;"
                              "}"
                              "QPushButton#stopPlotButton:pressed {"
                              "background-color: #a93226;"
                              "}";

    btn1min->setStyleSheet(buttonStyle);
    btn2min->setStyleSheet(buttonStyle);
    btn5min->setStyleSheet(buttonStyle);
    btnStopPlot->setStyleSheet(buttonStyle + stopButtonStyle);

    // 设置按钮固定宽度
    btn1min->setFixedWidth(80);
    btn2min->setFixedWidth(80);
    btn5min->setFixedWidth(80);
    btnStopPlot->setFixedWidth(80);

    btn1min->setEnabled(true);
    btn2min->setEnabled(true);
    btn5min->setEnabled(true);

    connect(btn1min, &QPushButton::clicked, [this]() {
        qDebug() << "1分钟按钮已点击";
        setTimeRange(OneMinute);
        });
    connect(btn2min, &QPushButton::clicked, [this]() { setTimeRange(TwoMinutes); });
    connect(btn5min, &QPushButton::clicked, [this]() { setTimeRange(FiveMinutes); });
    // 连接停止绘制按钮
    connect(btnStopPlot, &QPushButton::clicked, [this, btnStopPlot, buttonStyle, stopButtonStyle]() {
        isPlottingEnabled = !isPlottingEnabled;

        if (isPlottingEnabled) {
            btnStopPlot->setText("停止绘制");
            btnStopPlot->setStyleSheet(buttonStyle + stopButtonStyle); // 恢复红色样式
            // 恢复时更新图表到最新状态
            updateChartFromData();
        } else {
            btnStopPlot->setText("恢复绘制");
            btnStopPlot->setStyleSheet(buttonStyle); // 使用普通蓝色样式
        }
    });

    buttonLayout->addWidget(timeRangeLabel);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn1min);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn2min);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn5min);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnStopPlot); // 添加停止绘制按钮
    // 创建右侧布局（包含按钮和饼图）
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);
    rightLayout->addLayout(buttonLayout, 2.2);
    rightLayout->addWidget(pieChartContainer, 7.8);
    rightLayout->setSpacing(10);

    //    // 设置饼图容器背景
    pieChartContainer->setStyleSheet("background-color:white;");

     //将折线图和右侧布局添加到主布局
    mainLayout->addWidget(chartView, 8);   // 折线图占7份空间
    mainLayout->addLayout(rightLayout, 2); // 右侧部分占3份空间

    this->setLayout(mainLayout);

    flag = true;

    // 初始化数据存储
    dataPoints.clear();
    localDataPoints.clear(); // 初始化本地数据点存储
    
}
void Chart::updateChartFromData() {
    // 更新折线图
    series->clear();
    int startIdx = qMax(0, dataPoints.size() - CharLenth);
    for (int i = startIdx; i < dataPoints.size(); i++) {
        const auto& point = dataPoints[i];
        series->append(point.first.toMSecsSinceEpoch(), point.second);
    }
    // 更新本地数据曲线
    series_local->clear();
    int localStartIdx = qMax(0, localDataPoints.size() - CharLenth);
    for (int i = localStartIdx; i < localDataPoints.size(); i++) {
        const auto& point = localDataPoints[i];
        series_local->append(point.first.toMSecsSinceEpoch(), point.second);
    }
    // 更新坐标轴
    if (!dataPoints.isEmpty()) {
        QDateTime minTime = dataPoints[qMax(0, dataPoints.size() - CharLenth)].first;
        QDateTime maxTime = dataPoints.last().first.addSecs(8);
        xAxis->setRange(minTime, maxTime);
    }

    // 更新饼图
    updatePieChart();
}
// 设置时间范围
void Chart::setTimeRange(TimeRange range) {
    currentTimeRange = range;
    qDebug() << "时间范围切换为:" << range << "分钟";
    updatePieChart(); // 更新饼图
}

// 初始化饼图
void Chart::initPieChart() {
    // 创建饼图容器和布局
    pieChartContainer = new QWidget();
    QVBoxLayout* pieLayout = new QVBoxLayout(pieChartContainer);
    pieLayout->setContentsMargins(0, 0, 0, 0);
    pieLayout->setSpacing(0);

    // 创建紧凑容器
    QWidget* compactContainer = new QWidget();
    QVBoxLayout* compactLayout = new QVBoxLayout(compactContainer);
    compactLayout->setContentsMargins(0, 0, 0, 0);
    compactLayout->setSpacing(5);

    // 创建并设置清醒百分比标签
    clearPercentageLabel = new QLabel("清醒: 100%");
    clearPercentageLabel->setAlignment(Qt::AlignCenter);
    QFont clearFont = clearPercentageLabel->font();
    clearFont.setPointSize(18);
    clearFont.setBold(true);
    clearPercentageLabel->setFont(clearFont);
    clearPercentageLabel->setStyleSheet("color: #2ecc71; padding: 0;");

    // 创建并设置疲劳百分比标签
    fatiguePercentageLabel = new QLabel("疲劳: 0%");
    fatiguePercentageLabel->setAlignment(Qt::AlignCenter);
    QFont fatigueFont = fatiguePercentageLabel->font();
    fatigueFont.setPointSize(18);
    fatigueFont.setBold(true);
    fatiguePercentageLabel->setFont(fatigueFont);
    fatiguePercentageLabel->setStyleSheet("color: #e74c3c; padding: 0;");

    // 将标签添加到紧凑容器
    compactLayout->addWidget(clearPercentageLabel);
    compactLayout->addWidget(fatiguePercentageLabel);

    // 创建饼图系列
    pieSeries = new QPieSeries();
    pieSeries->append("清醒", 1);
    pieSeries->append("疲劳", 0);

    // 设置切片颜色
    if (pieSeries->slices().size() >= 2) {
        QPieSlice* clearSlice = pieSeries->slices().at(0);
        clearSlice->setColor(QColor("#2ecc71"));
        clearSlice->setLabelVisible(false);

        QPieSlice* fatigueSlice = pieSeries->slices().at(1);
        fatigueSlice->setColor(QColor("#e74c3c"));
        fatigueSlice->setLabelVisible(false);
    }

    // 创建饼图图表
    pieChart = new QChart();
    pieChart->addSeries(pieSeries);
    pieChart->setTitle(""); // 移除标题
    pieChart->legend()->setVisible(false); // 隐藏图例
    pieChart->setBackgroundVisible(false); // 透明背景
    pieChart->setMargins(QMargins(0, 0, 0, 0)); // 移除图表边距

    // 创建饼图视图
    pieChartView = new QChartView(pieChart);
    pieChartView->setRenderHint(QPainter::Antialiasing);
    pieChartView->setStyleSheet("background: transparent; border: none;");
    pieChartView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed); // 固定大小

    // 设置饼图尺寸
    int compactSize = 400; // 饼图直径
    pieChartView->setFixedSize(compactSize, compactSize);

    // 创建饼图容器，居中显示
    QWidget* chartContainer = new QWidget();
    chartContainer->setStyleSheet("background: transparent;");
    QHBoxLayout* chartContainerLayout = new QHBoxLayout(chartContainer);
    chartContainerLayout->setContentsMargins(0, 0, 0, 0);
    chartContainerLayout->addWidget(pieChartView, 0, Qt::AlignCenter);

    // 将饼图添加到紧凑容器
    compactLayout->addWidget(chartContainer, 0, Qt::AlignCenter);

    // 将紧凑容器添加到主布局
    pieLayout->addWidget(compactContainer, 0, Qt::AlignCenter);
}

// 更新饼图
void Chart::updatePieChart() {
    int clearCount = 0;
    int fatigueCount = 0;

    if (!dataPoints.isEmpty()) {
        // 获取当前时间
        QDateTime currentTime = QDateTime::currentDateTime();

        // 计算筛选时间点
        QDateTime filterTime = currentTime.addSecs(-currentTimeRange * 60);

        // 统计指定时间范围内的数据
        for (const QPair<QDateTime, qreal>& point : dataPoints) {
            if (point.first >= filterTime) {
                if (point.second == 25) clearCount++;
                else if (point.second == 75) fatigueCount++;
            }
        }
    }

    int total = clearCount + fatigueCount;
    QString rangeName;
    switch (currentTimeRange) {
    case OneMinute: rangeName = "1分钟"; break;
    case TwoMinutes: rangeName = "2分钟"; break;
    case FiveMinutes: rangeName = "5分钟"; break;
    }

    // 更新饼图标题
    pieChart->setTitle(rangeName + "数据统计");

    if (total > 0) {
        // 计算百分比
        qreal clearPercent = (static_cast<qreal>(clearCount) / total) * 100.0;
        qreal fatiguePercent = (static_cast<qreal>(fatigueCount) / total) * 100.0;

        // 更新标签文本
        clearPercentageLabel->setText(QString("清醒: %1%").arg(clearPercent, 0, 'f', 1));
        fatiguePercentageLabel->setText(QString("疲劳: %1%").arg(fatiguePercent, 0, 'f', 1));
    }
    else {
        clearPercentageLabel->setText("清醒: 0%");
        fatiguePercentageLabel->setText("疲劳: 0%");
    }

    // 更新饼图数据
    pieSeries->clear();
    pieSeries->append("清醒", clearCount);
    pieSeries->append("疲劳", fatigueCount);

    // 设置切片颜色
    if (pieSeries->slices().size() >= 2) {
        QPieSlice* clearSlice = pieSeries->slices().at(0);
        clearSlice->setColor(QColor("#2ecc71"));
        clearSlice->setLabelVisible(false);

        QPieSlice* fatigueSlice = pieSeries->slices().at(1);
        fatigueSlice->setColor(QColor("#e74c3c"));
        fatigueSlice->setLabelVisible(false);
    }
}

void Chart::receiveDatas() {


    if (!isPlottingEnabled) {
        // 存储数据但不更新图表
        QDateTime currentTime = QDateTime::currentDateTime();
        qreal value = (result == 0) ? 25 : 75;
        dataPoints.append({currentTime, value});
        // 同时存储本地标签数据
        // 添加条件：仅当本地数据有效(!= -1)时存储
        if (localresult != static_cast<uint8_t>(-1)) {
            qreal localvalue = (localresult == 0) ? 30 : 80;
            localDataPoints.append({ currentTime, localvalue });
        }
        // qDebug()<<"存储本地标签数据"<<localLabelData.isEmpty();
        // if (!localLabelData.isEmpty() && localLabelIndex < localLabelData.size()) {
        //     QPair<QDateTime, qreal> localPoint;
        //     localPoint.first = currentTime;
        //     localPoint.second = (localLabelData[localLabelIndex] == 0) ? 30 : 80;
        //     localDataPoints.append(localPoint);
        //     localLabelIndex++;
        // }
        return;
    }


    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();

    // 创建数据点
    QPair<QDateTime, qreal> point;
    point.first = currentTime;
    point.second = (result == 0) ? 25 : 75;
    // 添加到数据列表
    dataPoints.append(point);

    // 处理本地标签数据
    if (localresult != static_cast<uint8_t>(-1)) {
        QPair<QDateTime, qreal> localPoint;
        localPoint.first = currentTime;
        localPoint.second = (localresult == 0) ? 30 : 80;
        localDataPoints.append(localPoint);
    }
    // if (!localLabelData.isEmpty() ) {
    //     QPair<QDateTime, qreal> localPoint;
    //     localPoint.first = currentTime;
    //     localPoint.second = (localLabelData[localLabelIndex] == 0) ? 30 : 80;
    //     localDataPoints.append(localPoint);

    //     localLabelIndex++; // 移动到下一个本地数据点
    // }


    // 清理过期数据（保留最近5分钟）
    while (!dataPoints.isEmpty() &&
        dataPoints.first().first.secsTo(currentTime) > 5 * 60) {
        dataPoints.removeFirst();
    }
    while (!localDataPoints.isEmpty() &&
           localDataPoints.first().first.secsTo(currentTime) > 5 * 60) {
        localDataPoints.removeFirst();
    }
    // 更新折线图
    series->clear();

    // 添加数据点（仅显示最近15个点）
    int startIdx = qMax(0, dataPoints.size() - CharLenth);
    for (int i = startIdx; i < dataPoints.size(); i++) {
        const QPair<QDateTime, qreal>& point = dataPoints[i];
        series->append(point.first.toMSecsSinceEpoch(), point.second);
    }

    // 更新本地曲线
    series_local->clear();
    int localStartIdx = qMax(0, localDataPoints.size() - CharLenth);
    for (int i = localStartIdx; i < localDataPoints.size(); i++) {
        const QPair<QDateTime, qreal>& point = localDataPoints[i];
        series_local->append(point.first.toMSecsSinceEpoch(), point.second);
    }

    // 更新坐标轴范围
    if (!dataPoints.isEmpty()) {
        QDateTime minTime = dataPoints[qMax(0, dataPoints.size() - CharLenth)].first;
        QDateTime maxTime = dataPoints.last().first.addSecs(8);
        xAxis->setRange(minTime, maxTime);
    }

    // 更新饼图
    updatePieChart();
    update();
}

void Chart::localLabel(QList<int> label_local) {
    // 清空现有数据
    dataPoints.clear();

    // 添加新数据
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < label_local.size(); ++i) {
        if (i < CharLenth) {
            QPair<QDateTime, qreal> point;
            point.first = baseTime.addSecs(i * 8);
            point.second = (label_local[i] == 0) ? 25 : 75;
            dataPoints.append(point);
        }
    }

    // 更新折线图
    series->clear();
    for (int i = 0; i < dataPoints.size(); i++) {
        const QPair<QDateTime, qreal>& point = dataPoints[i];
        series->append(point.first.toMSecsSinceEpoch(), point.second);
    }

    // 更新坐标轴范围
    if (!dataPoints.isEmpty()) {
        QDateTime minTime = dataPoints.first().first;
        QDateTime maxTime = dataPoints.last().first.addSecs(8);
        xAxis->setRange(minTime, maxTime);
    }

    // 更新饼图
    updatePieChart();
}

void Chart::updateChart() {
    QDateTime currentTime = QDateTime::currentDateTime();
    // 删除最旧的数据点
    if (series->count() >= CharLenth) {
        series->remove(0);
    }
    // 更新横坐标范围
    QDateTime futureTime = currentTime.addSecs(8);
    xAxis->setRange(currentTime, futureTime);
}

void Chart::connectdata(quint8 data, uint8_t localdata) {
    if (flag) {//接收数据才初始化绘制图形窗口
        init();
        flag = false;
    }
    result = data;
    
    
    localresult = localdata;
    
    qDebug()<<"本地数据标签:"<<localresult;
    receiveDatas();
}
