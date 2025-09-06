#ifndef PLATFORMWIDGET_H
#define PLATFORMWIDGET_H

#include <QWidget>
#include <vector> // 添加vector支持
#include <Eigen> // 包含Eigen库
#include "ui_platformwidget.h"
#include "mainwidget.h"
#include "bciia.h"
#include "fatiguereswidget.h"
//#include "tskfatigue.h"
#include "preprocessing.h"
#include "prefcm.h" // FCM头文件
#include "chart.h"
#include "filestorage.h"
#include "mathgame.h"

#include"EEGProcessor.h"
#include"ICAProcessor.h"
#include"recognition.h"
#include "LogisticRegression.h"
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

	// EEG预处理
    
	EEGProcessor eegpro{ 500, 250, 2 }; // 假设有2个通道，FIR滤波器阶数为101
	ICAProcessor icaProc;
    LogisticRegression clf;
    int label_index;
    // 信号处理
    Preprocessing pre;

    // FCM模糊聚类
    preFcm m_preFcm;

    // 疲劳检测模型
    //Calculate::TskFatigue tskfatifue;
	Calculate::Recognition recognition;
    //私有变量
    int coarse_predict_1;
    int fine_predict_1;
    int coarse_predict_2;
    int fine_predict_2;
    int origin_predict1;
    int origin_predict2;
    int convert_original_label_1(int coarse, int fine);
    int convert_original_label_2(int coarse, int fine);






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
    int is_ceived = 0;
    int localLabelIndex;

};

#endif // PLATFORMWIDGET_H
