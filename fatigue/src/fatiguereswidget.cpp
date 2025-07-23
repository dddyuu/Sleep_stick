#include "fatiguereswidget.h"
#include "ui_fatiguereswidget.h"
#include <QResizeEvent>
#include <QShowEvent>
#include <QDebug>

FatigueResWidget::FatigueResWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::FatigueResWidget)
{
    ui->setupUi(this);

    // 获取图表控件指针
    chart_index = ui->chart;

    // 创建注册信息面板（不再通过 UI 文件获取）
    reg = new registerInfo(this);
    reg->setMaximumSize(316, 200); // 设置最大尺寸

    // 确保信息面板显示在图表上方
    reg->raise();
    // reg->lower(); // 将 registerInfo 窗口降低到按钮下方
    reg->show();

    // 被试信息保存
    connect(reg,&registerInfo::sendSubname,this,&FatigueResWidget::transSubname);
    connect(this, &FatigueResWidget::sendData, chart_index, &Chart::connectdata);//这个作为转发数据，需要插入设备且发送数据才用

    //connect(reg,&registerInfo::sendLabel,chart_index,&Chart::localLabel);

//    Infomation.clear();

    // 初始定位（如果图表已初始化）
    QTimer::singleShot(100, this, &FatigueResWidget::positionRegisterInfo);
    // 确保按钮可点击
    chart_index->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chart_index->raise();
}

FatigueResWidget::~FatigueResWidget()
{
    delete ui;
}

void FatigueResWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    positionRegisterInfo();
}

void FatigueResWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    positionRegisterInfo();
}

void FatigueResWidget::positionRegisterInfo()
{
    if (chart_index && reg && chart_index->isVisible()) {
        // 获取图表在窗口中的位置
        QPoint chartTopRight = chart_index->mapTo(this, QPoint(chart_index->width(), 0));

        // 计算信息面板的位置（图表右上角）
        int x = chartTopRight.x() - reg->width() - 10; // 减去10像素的边距
        int y = chartTopRight.y() + 10; // 加上10像素的边距

        // 设置位置
        reg->move(x, y);
    }
}
//void FatigueResWidget::positionRegisterInfo()
//{
//    if (chart_index && reg && chart_index->isVisible()) {
//        // 获取图表在窗口中的位置
//        QPoint chartBottomRight = chart_index->mapTo(this, QPoint(chart_index->width(), chart_index->height()));
//
//        // 计算信息面板的位置（图表右下角）
//        int x = chartBottomRight.x() - reg->width(); // 减去信息面板的宽度
//        int y = chartBottomRight.y() - reg->height() - 10; // 减去信息面板的高度和10像素的边距
//
//        // 设置位置
//        reg->move(x, y);
//    }
//}
void FatigueResWidget::on_fatigueClass_clicked()
{
    //显示疲劳等级
}
