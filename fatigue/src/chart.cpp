#include "chart.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>

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
    mediumPercentageLabel = nullptr;
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
}

void Chart::init() {
    QHBoxLayout* mainLayout = new QHBoxLayout();// 创建主水平布局

    chart1 = new QChart();
    chart1->setTitle("认知状态监测");

    // 初始化线条
    series = new QLineSeries;
    series->setName("实时数据");
    {
        QPen pen1(Qt::blue);
        pen1.setWidth(2);
        series->setPen(pen1);
    }
    chart1->addSeries(series);

    // 初始化本地数据线条
    series_local = new QLineSeries;
    series_local->setName("本地标签");
    {
        QPen pen2(Qt::red);
        pen2.setWidth(2);
        pen2.setStyle(Qt::DashLine); // 虚线
        series_local->setPen(pen2);
    }
    chart1->addSeries(series_local);

    // 横坐标（改为点数索引）
    xAxis = new QValueAxis;
    xAxis->setLabelFormat("%d");
    xAxis->setTitleText("点数");
    xAxis->setRange(1, 10);               // 初始给出可见范围
    xAxis->setTickCount(5);               // 至少2，给定一个合理初值
    chart1->addAxis(xAxis, Qt::AlignBottom);
    series->attachAxis(xAxis);
    series_local->attachAxis(xAxis);

    // 纵坐标
    QCategoryAxis* axisY = new QCategoryAxis;
    axisY->setMin(0);
    axisY->setMax(100);
    axisY->setStartValue(0);
    axisY->append("低负荷", 25);
    axisY->append("中负荷", 75);
    axisY->append("高负荷", 90);
    axisY->setGridLineVisible(false); // 不要自动画网格
    chart1->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    series_local->attachAxis(axisY);

    // 再加一个数值型坐标轴（只负责画两根线）
    QValueAxis* axisY2 = new QValueAxis();
    axisY2->setRange(0, 100);
    axisY2->setTickCount(0);          // 不显示普通刻度
    axisY2->setLabelsVisible(false);  // 不显示数字
    chart1->addAxis(axisY2, Qt::AlignRight);

    // 添加图例
    chart1->legend()->setVisible(true);
    chart1->legend()->setAlignment(Qt::AlignBottom);

    // 关键修复：创建 chartView 并启用抗锯齿
    chartView = new QChartView(chart1);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumSize(400, 300);

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

    // 添加导出按钮
    QPushButton* btnSaveExcel = new QPushButton("导出Excel");
    btnSaveExcel->setObjectName("saveExcelButton");

    QPushButton* btnSaveChart = new QPushButton("导出图表");
    btnSaveChart->setObjectName("saveChartButton");

    QPushButton* btnSaveAll = new QPushButton("导出全部");
    btnSaveAll->setObjectName("saveAllButton");

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
        "background-color: #e74c3c;"
        "}"
        "QPushButton#stopPlotButton:hover {"
        "background-color: #c0392b;"
        "}"
        "QPushButton#stopPlotButton:pressed {"
        "background-color: #a93226;"
        "}";

    // Excel按钮特殊样式
    QString excelButtonStyle = "QPushButton#saveExcelButton {"
        "background-color: #27ae60;"
        "}"
        "QPushButton#saveExcelButton:hover {"
        "background-color: #229954;"
        "}"
        "QPushButton#saveExcelButton:pressed {"
        "background-color: #1e8449;"
        "}";

    // 图表按钮特殊样式
    QString chartButtonStyle = "QPushButton#saveChartButton {"
        "background-color: #f39c12;"
        "}"
        "QPushButton#saveChartButton:hover {"
        "background-color: #d68910;"
        "}"
        "QPushButton#saveChartButton:pressed {"
        "background-color: #b7950b;"
        "}";

    // 全部按钮特殊样式
    QString allButtonStyle = "QPushButton#saveAllButton {"
        "background-color: #9b59b6;"
        "}"
        "QPushButton#saveAllButton:hover {"
        "background-color: #8e44ad;"
        "}"
        "QPushButton#saveAllButton:pressed {"
        "background-color: #7d3c98;"
        "}";

    btn1min->setStyleSheet(buttonStyle);
    btn2min->setStyleSheet(buttonStyle);
    btn5min->setStyleSheet(buttonStyle);
    btnStopPlot->setStyleSheet(buttonStyle + stopButtonStyle);
    btnSaveExcel->setStyleSheet(buttonStyle + excelButtonStyle);
    btnSaveChart->setStyleSheet(buttonStyle + chartButtonStyle);
    btnSaveAll->setStyleSheet(buttonStyle + allButtonStyle);

    // 设置按钮固定宽度
    btn1min->setFixedWidth(80);
    btn2min->setFixedWidth(80);
    btn5min->setFixedWidth(80);
    btnStopPlot->setFixedWidth(80);
    btnSaveExcel->setFixedWidth(80);
    btnSaveChart->setFixedWidth(80);
    btnSaveAll->setFixedWidth(80);

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
            btnStopPlot->setStyleSheet(buttonStyle + stopButtonStyle);
            updateChartFromData();
        }
        else {
            btnStopPlot->setText("恢复绘制");
            btnStopPlot->setStyleSheet(buttonStyle);
        }
        });

    // 连接按钮信号
    connect(btnSaveExcel, &QPushButton::clicked, this, &Chart::saveDataToExcel);
    connect(btnSaveChart, &QPushButton::clicked, this, &Chart::saveChartToImage);
    connect(btnSaveAll, &QPushButton::clicked, this, &Chart::saveDataAndChart);

    // 右侧：按钮 + 饼图
    //QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(5);
    buttonLayout->addWidget(timeRangeLabel);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn1min);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn2min);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btn5min);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnStopPlot);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(btnSaveExcel);
    buttonLayout->addSpacing(5);
    buttonLayout->addWidget(btnSaveChart);
    buttonLayout->addSpacing(5);
    buttonLayout->addWidget(btnSaveAll);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);
    rightLayout->addLayout(buttonLayout, 2);
    rightLayout->addWidget(pieChartContainer, 8);
    rightLayout->setSpacing(10);

    // 设置饼图容器背景
    pieChartContainer->setStyleSheet("background-color:white;");

    // 将折线图和右侧布局添加到主布局
    mainLayout->addWidget(chartView, 8);
    mainLayout->addLayout(rightLayout, 2);

    this->setLayout(mainLayout);

    flag = true;

    // 初始化数据存储
    dataPoints.clear();
    localDataPoints.clear();
    comparisonData.clear();
}

void Chart::saveDataToExcel() {
    if (comparisonData.isEmpty()) {
        QMessageBox::information(this, "提示", "暂无数据可导出");
        return;
    }

    QString filePath = getExcelFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    if (saveDataToFile(filePath)) {
        int totalRecords = comparisonData.size();
        int validComparisons = 0;
        int matchingRecords = 0;

        for (const DataRecord& record : comparisonData) {
            if (record.hasActualValue) {
                validComparisons++;
                bool isMatch = qAbs(record.predictedValue - record.actualValue) <= 5.0;
                if (isMatch) matchingRecords++;
            }
        }

        QString message = QString("数据已成功导出到:\n%1\n\n统计信息:\n总记录数: %2\n有效对比数: %3")
            .arg(filePath)
            .arg(totalRecords)
            .arg(validComparisons);

        if (validComparisons > 0) {
            double accuracy = (double)matchingRecords / validComparisons * 100.0;
            message += QString("\n准确率: %1%").arg(QString::number(accuracy, 'f', 2));
        }

        QMessageBox::information(this, "导出成功", message);
    }
    else {
        QMessageBox::critical(this, "导出失败", QString("无法保存文件到:\n%1").arg(filePath));
    }
}

void Chart::saveChartToImage() {
    if (dataPoints.isEmpty() && localDataPoints.isEmpty()) {
        QMessageBox::information(this, "提示", "暂无图表数据可导出");
        return;
    }

    QString filePath = getChartImageFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QPixmap chartPixmap = createChartImage();

    if (chartPixmap.save(filePath)) {
        QMessageBox::information(this, "导出成功",
            QString("图表已成功导出到:\n%1").arg(filePath));
    }
    else {
        QMessageBox::critical(this, "导出失败",
            QString("无法保存图表到:\n%1").arg(filePath));
    }
}

void Chart::saveDataAndChart() {
    if (comparisonData.isEmpty()) {
        QMessageBox::information(this, "提示", "暂无数据可导出");
        return;
    }

    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir dir(documentsPath);
    if (!dir.exists("CognitiveStateData")) {
        dir.mkpath("CognitiveStateData");
    }

    QString saveDir = QFileDialog::getExistingDirectory(
        this,
        "选择导出目录",
        dir.absoluteFilePath("CognitiveStateData")
    );

    if (saveDir.isEmpty()) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    QString excelPath = QDir(saveDir).absoluteFilePath(
        QString("认知状态数据_%1.csv").arg(timestamp));

    QString chartPath = QDir(saveDir).absoluteFilePath(
        QString("认知状态图表_%1.png").arg(timestamp));

    bool excelSuccess = saveDataToFile(excelPath);
    bool chartSuccess = false;

    if (excelSuccess) {
        QPixmap chartPixmap = createChartImage();
        chartSuccess = chartPixmap.save(chartPath);
    }

    QString message;
    if (excelSuccess && chartSuccess) {
        message = QString("数据和图表已成功导出到:\n数据文件: %1\n图表文件: %2")
            .arg(excelPath).arg(chartPath);
        QMessageBox::information(this, "导出成功", message);
    }
    else {
        message = "导出过程中发生错误:\n";
        if (!excelSuccess) message += "- 数据文件保存失败\n";
        if (!chartSuccess) message += "- 图表文件保存失败\n";
        QMessageBox::critical(this, "导出失败", message);
    }
}

QString Chart::getExcelFilePath() {
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QDir dir(documentsPath);
    if (!dir.exists("CognitiveStateData")) {
        dir.mkpath("CognitiveStateData");
    }

    QString defaultFileName = QString("认知状态对比数据_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString defaultPath = dir.absoluteFilePath("CognitiveStateData/" + defaultFileName);

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存认知状态对比数据",
        defaultPath,
        "CSV文件 (*.csv);;Excel文件 (*.xlsx);;所有文件 (*.*)"
    );

    return filePath;
}

QString Chart::getChartImageFilePath() {
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QDir dir(documentsPath);
    if (!dir.exists("CognitiveStateData")) {
        dir.mkpath("CognitiveStateData");
    }

    QString defaultFileName = QString("认知状态完整对比图表_%1.png")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString defaultPath = dir.absoluteFilePath("CognitiveStateData/" + defaultFileName);

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存认知状态完整对比图表",
        defaultPath,
        "PNG图像 (*.png);;JPEG图像 (*.jpg);;BMP图像 (*.bmp);;所有文件 (*.*)"
    );

    return filePath;
}

// 新增：创建图表图像（使用索引轴）
QPixmap Chart::createChartImage() {
    int totalPoints = qMax(dataPoints.size(), localDataPoints.size());
    int chartWidth = qMax(1600, totalPoints * 20);
    int chartHeight = 900;

    QChart* tempChart = new QChart();
    tempChart->setTitle("认知状态监测完整对比图表");

    tempChart->layout()->setContentsMargins(50, 50, 50, 200);

    QLineSeries* tempSeries = new QLineSeries();
    tempSeries->setName("实时数据");
    {
        QPen pen1(Qt::blue);
        pen1.setWidth(3);
        tempSeries->setPen(pen1);
    }

    QLineSeries* tempSeriesLocal = new QLineSeries();
    tempSeriesLocal->setName("本地标签");
    {
        QPen pen2(Qt::red);
        pen2.setWidth(3);
        pen2.setStyle(Qt::DashLine);
        tempSeriesLocal->setPen(pen2);
    }

    // 按索引填充 x = i + 1
    for (int i = 0; i < dataPoints.size(); ++i) {
        tempSeries->append(i + 1, dataPoints[i].second);
    }
    for (int i = 0; i < localDataPoints.size(); ++i) {
        tempSeriesLocal->append(i + 1, localDataPoints[i].second);
    }

    tempChart->addSeries(tempSeries);
    tempChart->addSeries(tempSeriesLocal);

    // 索引轴（范围与刻度随数据量调整）
    QValueAxis* tempXAxis = new QValueAxis();
    tempXAxis->setLabelFormat("%d");
    tempXAxis->setTitleText("点数");
    int end = qMax(10, qMin(MaxPoints, totalPoints > 0 ? totalPoints : 10));
    tempXAxis->setRange(1, end);
    tempXAxis->setTickCount(qMax(2, qMin(15, end / 10 + 1)));
    tempChart->addAxis(tempXAxis, Qt::AlignBottom);
    tempSeries->attachAxis(tempXAxis);
    tempSeriesLocal->attachAxis(tempXAxis);

    // Y 轴
    QCategoryAxis* tempAxisY = new QCategoryAxis();
    tempAxisY->setMin(0);
    tempAxisY->setMax(100);
    tempAxisY->setStartValue(0);
    tempAxisY->append("低负荷", 25);
    tempAxisY->append("中负荷", 75);
    tempAxisY->append("高负荷", 90);
    tempAxisY->setGridLineVisible(true);
    tempAxisY->setTitleText("认知负荷状态");
    tempChart->addAxis(tempAxisY, Qt::AlignLeft);
    tempSeries->attachAxis(tempAxisY);
    tempSeriesLocal->attachAxis(tempAxisY);

    tempChart->legend()->setVisible(true);
    tempChart->legend()->setAlignment(Qt::AlignBottom);
    {
        QFont legendFont = tempChart->legend()->font();
        legendFont.setPointSize(12);
        legendFont.setBold(true);
        tempChart->legend()->setFont(legendFont);
    }

    QPixmap pixmap(chartWidth, chartHeight);
    pixmap.fill(Qt::white);

    QChartView* tempView = new QChartView(tempChart);
    tempView->setRenderHint(QPainter::Antialiasing);
    tempView->resize(chartWidth, chartHeight - 180);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF chartRect(0, 0, chartWidth, chartHeight - 180);
    tempView->render(&painter, chartRect);

    // 添加统计信息
    QString statsText = generateStatsText();
    if (!statsText.isEmpty()) {
        QStringList lines = statsText.split('\n');
        int statsY = chartHeight - 170;
        int statsX = 50;

        painter.setFont(QFont("Microsoft YaHei", 11));
        painter.setPen(Qt::darkGray);

        for (const QString& line : lines) {
            painter.drawText(statsX, statsY, line);
            statsY += 20;
        }
    }

    // 添加时间戳
    painter.setFont(QFont("Microsoft YaHei", 10));
    painter.setPen(Qt::gray);
    QString timestamp = QString("导出时间: %1 | 数据点数: %2")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(totalPoints);
    painter.drawText(50, chartHeight - 20, timestamp);

    painter.end();

    delete tempView;

    return pixmap;
}

// 新增：生成统计信息文本
QString Chart::generateStatsText() {
    if (comparisonData.isEmpty()) {
        return "暂无统计数据";
    }

    int totalRecords = 0;
    int matchingRecords = 0;
    double totalError = 0.0;
    int validComparisons = 0;

    for (const DataRecord& record : comparisonData) {
        totalRecords++;
        if (record.hasActualValue) {
            bool isMatch = qAbs(record.predictedValue - record.actualValue) <= 5.0;
            if (isMatch) matchingRecords++;
            totalError += qAbs(record.predictedValue - record.actualValue);
            validComparisons++;
        }
    }

    QString stats = QString("=== 数据统计 ===\n总记录数: %1\n有效对比数: %2")
        .arg(totalRecords).arg(validComparisons);

    if (validComparisons > 0) {
        double accuracy = (double)matchingRecords / validComparisons * 100.0;
        double avgError = totalError / validComparisons;
        stats += QString("\n匹配记录数: %1\n准确率: %2%\n平均误差: %3")
            .arg(matchingRecords)
            .arg(QString::number(accuracy, 'f', 2))
            .arg(QString::number(avgError, 'f', 2));
    }

    return stats;
}

// 新增：将数据保存到指定文件的辅助方法
bool Chart::saveDataToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream.setGenerateByteOrderMark(true);

    // 写入标题行
    stream << QString::fromUtf8("时间戳,预测值,预测标签,真实值,真实标签,是否匹配,差值\n");

    // 统计数据
    int totalRecords = 0;
    int matchingRecords = 0;
    double totalError = 0.0;
    int validComparisons = 0;

    // 写入数据行
    for (const DataRecord& record : comparisonData) {
        totalRecords++;

        QString line = record.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") + ",";
        line += QString::number(record.predictedValue, 'f', 1) + ",";
        line += record.predictedLabel + ",";

        if (record.hasActualValue) {
            line += QString::number(record.actualValue, 'f', 1) + ",";
            line += record.actualLabel + ",";
            bool isMatch = qAbs(record.predictedValue - record.actualValue) <= 5.0;
            line += QString(isMatch ? "匹配" : "不匹配") + ",";
            double diff = record.predictedValue - record.actualValue;
            line += QString::number(diff, 'f', 1);

            if (isMatch) matchingRecords++;
            totalError += qAbs(diff);
            validComparisons++;
        }
        else {
            line += "无真实值,无真实标签,无法比较,无法计算";
        }

        stream << QString::fromUtf8(line.toUtf8()) << "\n";
    }

    // 写入统计信息
    stream << "\n" << QString::fromUtf8("=== 统计信息 ===\n");
    stream << QString::fromUtf8("总记录数: %1\n").arg(totalRecords);
    stream << QString::fromUtf8("有效对比数: %1\n").arg(validComparisons);

    if (validComparisons > 0) {
        double accuracy = (double)matchingRecords / validComparisons * 100.0;
        double avgError = totalError / validComparisons;

        stream << QString::fromUtf8("匹配记录数: %1\n").arg(matchingRecords);
        stream << QString::fromUtf8("准确率: %1%\n").arg(QString::number(accuracy, 'f', 2));
        stream << QString::fromUtf8("平均绝对误差: %1\n").arg(QString::number(avgError, 'f', 2));
    }

    file.close();
    return true;
}

// 将数值转换为标签描述
QString Chart::valueToLabel(qreal value) {
    if (value <= 30) {
        return "低负荷";
    }
    else if (value <= 70) {
        return "中负荷";
    }
    else {
        return "高负荷";
    }
}

// 添加对比记录
void Chart::addComparisonRecord(QDateTime timestamp, qreal predicted, qreal actual) {
    DataRecord record;
    record.timestamp = timestamp;
    record.predictedValue = predicted;
    record.predictedLabel = valueToLabel(predicted);

    if (actual >= 0) {
        record.actualValue = actual;
        record.actualLabel = valueToLabel(actual);
        record.hasActualValue = true;
    }
    else {
        record.hasActualValue = false;
    }

    comparisonData.append(record);

    // 限制数据量，避免内存占用过大
    if (comparisonData.size() > 10000) {
        comparisonData.removeFirst();
    }
}

void Chart::updateChartFromData() {
    // 保留最多 MaxPoints
    while (dataPoints.size() > MaxPoints) dataPoints.removeFirst();
    while (localDataPoints.size() > MaxPoints) localDataPoints.removeFirst();

    series->clear();
    for (int i = 0; i < dataPoints.size(); i++) {
        series->append(i + 1, dataPoints[i].second);
    }
    series_local->clear();
    for (int i = 0; i < localDataPoints.size(); i++) {
        series_local->append(i + 1, localDataPoints[i].second);
    }

    // 根据当前数据量调整横轴显示范围与刻度
    int count = qMax(dataPoints.size(), localDataPoints.size());
    int end = qMax(10, qMin(MaxPoints, count > 0 ? count : 10));
    xAxis->setRange(1, end);
    xAxis->setTickCount(qMax(2, qMin(15, end / 10 + 1)));

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

    // 创建并设置低/中/高负荷百分比标签
    clearPercentageLabel = new QLabel("低负荷: 100%");
    clearPercentageLabel->setAlignment(Qt::AlignCenter);
    {
        QFont clearFont = clearPercentageLabel->font();
        clearFont.setPointSize(18);
        clearFont.setBold(true);
        clearPercentageLabel->setFont(clearFont);
    }
    clearPercentageLabel->setStyleSheet("color: #2ecc71; padding: 0;");

    mediumPercentageLabel = new QLabel("中负荷: 0%");
    mediumPercentageLabel->setAlignment(Qt::AlignCenter);
    {
        QFont midFont = mediumPercentageLabel->font();
        midFont.setPointSize(18);
        midFont.setBold(true);
        mediumPercentageLabel->setFont(midFont);
    }
    mediumPercentageLabel->setStyleSheet("color: #f1c40f; padding: 0;");

    fatiguePercentageLabel = new QLabel("高负荷: 0%");
    fatiguePercentageLabel->setAlignment(Qt::AlignCenter);
    {
        QFont fatigueFont = fatiguePercentageLabel->font();
        fatigueFont.setPointSize(18);
        fatigueFont.setBold(true);
        fatiguePercentageLabel->setFont(fatigueFont);
    }
    fatiguePercentageLabel->setStyleSheet("color: #e74c3c; padding: 0;");

    // 将标签添加到紧凑容器
    compactLayout->addWidget(clearPercentageLabel);
    compactLayout->addWidget(mediumPercentageLabel);
    compactLayout->addWidget(fatiguePercentageLabel);

    // 创建饼图系列（三类）
    pieSeries = new QPieSeries();
    pieSeries->append("低负荷", 1);
    pieSeries->append("中负荷", 0);
    pieSeries->append("高负荷", 0);

    // 设置切片颜色
    if (pieSeries->slices().size() >= 3) {
        QPieSlice* clearSlice = pieSeries->slices().at(0);
        clearSlice->setColor(QColor("#2ecc71"));
        clearSlice->setLabelVisible(false);

        QPieSlice* midSlice = pieSeries->slices().at(1);
        midSlice->setColor(QColor("#f1c40f"));
        midSlice->setLabelVisible(false);

        QPieSlice* fatigueSlice = pieSeries->slices().at(2);
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
    int mediumCount = 0;
    int fatigueCountLocal = 0;

    if (!dataPoints.isEmpty()) {
        // 获取当前时间
        QDateTime currentTime = QDateTime::currentDateTime();

        // 计算筛选时间点
        QDateTime filterTime = currentTime.addSecs(-currentTimeRange * 60);

        // 统计指定时间范围内的数据
        for (const QPair<QDateTime, qreal>& point : dataPoints) {
            if (point.first >= filterTime) {
                // 与 valueToLabel 的阈值一致：<=30 低，<=70 中，其余高
                if (point.second <= 30) clearCount++;
                else if (point.second <= 70) mediumCount++;
                else fatigueCountLocal++;
            }
        }
    }

    int total = clearCount + mediumCount + fatigueCountLocal;
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
        qreal mediumPercent = (static_cast<qreal>(mediumCount) / total) * 100.0;
        qreal fatiguePercent = (static_cast<qreal>(fatigueCountLocal) / total) * 100.0;

        // 更新标签文本
        clearPercentageLabel->setText(QString("低负荷: %1%").arg(clearPercent, 0, 'f', 1));
        mediumPercentageLabel->setText(QString("中负荷: %1%").arg(mediumPercent, 0, 'f', 1));
        fatiguePercentageLabel->setText(QString("高负荷: %1%").arg(fatiguePercent, 0, 'f', 1));
    }
    else {
        clearPercentageLabel->setText("低负荷: 0%");
        mediumPercentageLabel->setText("中负荷: 0%");
        fatiguePercentageLabel->setText("高负荷: 0%");
    }

    // 更新饼图数据
    pieSeries->clear();
    pieSeries->append("低负荷", clearCount);
    pieSeries->append("中负荷", mediumCount);
    pieSeries->append("高负荷", fatigueCountLocal);

    // 设置切片颜色
    if (pieSeries->slices().size() >= 3) {
        QPieSlice* clearSlice = pieSeries->slices().at(0);
        clearSlice->setColor(QColor("#2ecc71"));
        clearSlice->setLabelVisible(false);

        QPieSlice* midSlice = pieSeries->slices().at(1);
        midSlice->setColor(QColor("#f1c40f"));
        midSlice->setLabelVisible(false);

        QPieSlice* fatigueSlice = pieSeries->slices().at(2);
        fatigueSlice->setColor(QColor("#e74c3c"));
        fatigueSlice->setLabelVisible(false);
    }
}

void Chart::receiveDatas() {
    if (!isPlottingEnabled) {
        // 存储数据但不更新图表
        QDateTime currentTime = QDateTime::currentDateTime();
        qreal value;
        if (result == 0) {
            value = 25;
        }
        else if (result == 1) {
            value = 55;
        }
        else {
            value = 85;
        }
        dataPoints.append({ currentTime, value });

        // 同时存储本地标签数据
        qreal localvalue = -1;
        if (localresult != static_cast<uint8_t>(-1)) {
            if (localresult == 0) {
                localvalue = 30;
            }
            else if (localresult == 1) {
                localvalue = 60;
            }
            else {
                localvalue = 90;
            }
            localDataPoints.append({ currentTime, localvalue });
        }

        // 限制总点数
        while (dataPoints.size() > MaxPoints) dataPoints.removeFirst();
        while (localDataPoints.size() > MaxPoints) localDataPoints.removeFirst();

        // 添加对比记录到Excel数据中
        addComparisonRecord(currentTime, value, localvalue);

        return;
    }

    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();

    // 创建数据点
    QPair<QDateTime, qreal> point;
    point.first = currentTime;
    if (result == 0) {
        point.second = 25;
    }
    else if (result == 1) {
        point.second = 55;
    }
    else {
        point.second = 85;
    }
    // 添加到数据列表
    dataPoints.append(point);

    // 处理本地标签数据
    qreal localvalue = -1;
    if (localresult != static_cast<uint8_t>(-1)) {
        QPair<QDateTime, qreal> localPoint;
        localPoint.first = currentTime;
        if (localresult == 0) {
            localPoint.second = 30;
            localvalue = 30;
        }
        else if (localresult == 1) {
            localPoint.second = 60;
            localvalue = 60;
        }
        else {
            localPoint.second = 90;
            localvalue = 90;
        }
        localDataPoints.append(localPoint);
    }

    // 限制总点数为 MaxPoints
    while (dataPoints.size() > MaxPoints) dataPoints.removeFirst();
    while (localDataPoints.size() > MaxPoints) localDataPoints.removeFirst();

    // 添加对比记录到Excel数据中
    addComparisonRecord(currentTime, point.second, localvalue);

    // 更新折线图（使用索引作为横坐标）
    series->clear();
    for (int i = 0; i < dataPoints.size(); i++) {
        series->append(i + 1, dataPoints[i].second);
    }

    // 更新本地曲线
    series_local->clear();
    for (int i = 0; i < localDataPoints.size(); i++) {
        series_local->append(i + 1, localDataPoints[i].second);
    }

    // 根据当前数据量调整横轴显示范围与刻度
    int count = qMax(dataPoints.size(), localDataPoints.size());
    int end = qMax(10, qMin(MaxPoints, count > 0 ? count : 10));
    xAxis->setRange(1, end);
    xAxis->setTickCount(qMax(2, qMin(15, end / 10 + 1)));

    // 更新饼图
    updatePieChart();
    update();
}

void Chart::localLabel(QList<int> label_local) {
    // 清空现有数据
    dataPoints.clear();

    // 添加新数据（索引填充）
    QDateTime baseTime = QDateTime::currentDateTime();
    for (int i = 0; i < label_local.size(); ++i) {
        if (i < MaxPoints) {
            QPair<QDateTime, qreal> point;
            point.first = baseTime.addMSecs(i); // 时间占位，不影响作图
            point.second = (label_local[i] == 0) ? 25 : 75;
            dataPoints.append(point);
        }
    }

    // 更新折线图
    series->clear();
    for (int i = 0; i < dataPoints.size(); i++) {
        series->append(i + 1, dataPoints[i].second);
    }

    // 根据当前数据量调整横轴显示范围与刻度
    int count = dataPoints.size();
    int end = qMax(10, qMin(MaxPoints, count > 0 ? count : 10));
    xAxis->setRange(1, end);
    xAxis->setTickCount(qMax(2, qMin(15, end / 10 + 1)));

    // 更新饼图
    updatePieChart();
}

void Chart::updateChart() {
    // 根据当前数据量调整横轴显示范围与刻度
    int count = qMax(dataPoints.size(), localDataPoints.size());
    int end = qMax(10, qMin(MaxPoints, count > 0 ? count : 10));
    xAxis->setRange(1, end);
    xAxis->setTickCount(qMax(2, qMin(15, end / 10 + 1)));
}

void Chart::connectdata(quint8 data, uint8_t localdata) {
    if (flag) {
        init();
        flag = false;
    }
    result = data;
    localresult = localdata;
    receiveDatas();
}

// 空实现保持接口兼容性
void Chart::updatePercentageLabels() {
    // 此方法已通过updatePieChart()实现，保留空实现用于兼容性
}