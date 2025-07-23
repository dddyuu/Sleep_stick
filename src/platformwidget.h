#ifndef PLATFORMWIDGET_H
#define PLATFORMWIDGET_H

#include <QWidget>
#include <vector> // 添加vector支持
#include <Eigen> // 包含Eigen库
#include "ui_platformwidget.h"
#include "mainwidget.h"
#include "bciia.h"
#include "fatiguereswidget.h"
#include "tskfatigue.h"
#include "preprocessing.h"
#include "prefcm.h" // FCM头文件
#include "chart.h"
#include "filestorage.h"
#include "mathgame.h"
namespace Ui {
    class PlatFormWidget;
}

class PlatFormWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlatFormWidget(QWidget* parent = nullptr);
    ~PlatFormWidget();
signals:
    void result_send(quint8 result, uint8_t localresult);
    void resultsave_send(QList<uint8_t> resultsave);
private:
    Ui::PlatFormWidget* ui;

    // 主页
    MainWidget* mainwidget;

    // 采集
    BCIIA bciia;

    // 结果
    FatigueResWidget* fatiguereswidget;
    MathGame* mathgame;
    registerInfo* regis;
    Chart* chart_index;
    FileStorage* fileStorage;
    // 信号处理
    Preprocessing pre;

    // FCM模糊聚类
    preFcm m_preFcm;

    // 疲劳检测模型
    Calculate::TskFatigue tskfatifue;

    // FCM训练状态
    bool m_fcmTrained;                         // 训练完成标志
    int m_trainingCounter;                     // 训练数据计数器
    std::vector<std::vector<float>> m_trainingData; // 训练数据存储

    // 私有方法
    void initWidget();
    void setConnect();
    void initFcmCenters();  // 初始化FCM聚类中心
    void saveFcmCenters();  // 保存FCM聚类中心
    QList<uint8_t>xx1;
    QList<uint8_t>saveResult;
    int is_ceived=0;
    int localLabelIndex;

};

#endif // PLATFORMWIDGET_H
