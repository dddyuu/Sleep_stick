#include "mishow.h"
#include "ui_mishow.h"
#include "QInputDialog"
#include "QMessageBox"
MIShow::MIShow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MIShow),
    dist(0, 1)
{
    ui->setupUi(this);
    initConnection();
    init();
    initControlWidget();
}

MIShow::~MIShow()
{
    delete ui;
    delete controlwidget;
}

void MIShow::initConnection()
{

}

void MIShow::init()
{

    QString str = "MI实验分为三个阶段：\n\n\
第一阶段：0-2s，休息准备期。在该阶段受试者可以休息，身体动作小，眨眼。\n\n\
第二阶段：2-4s，提示阶段。在该阶段，左手或右手移动动画会出现在屏幕上，以提醒受试者接下来的手部任务。\n\n\
第三阶段：4-8s，MI过程。在此期间，受试者根据监视器中的箭头提示，在相应方向上执行手移动的MI任务。\n\n";
    this->lab_description = new QLabel();
    this->lab_startTip = new QLabel();


    lab_description->setText(str);
    lab_description->setAlignment(Qt::AlignCenter);
    lab_description->setStyleSheet("QLabel{font-size:34px;font:bold}");

    this->lab_startTip->setText(">按空格键开始<\n");
    this->lab_startTip->setAlignment(Qt::AlignCenter);
    this->lab_startTip->setStyleSheet("QLabel{font-size:34px;font:bold}");

    // 设置动画
    setAnimation();

    // 设置布局，使label在屏幕中央
    layout_V = new QVBoxLayout();

    this->setLayout(layout_V);
    layout_V->addWidget(this->lab_description);
    layout_V->addWidget(this->lab_startTip);
    layout_V->setStretchFactor(this->lab_description, 8);
    layout_V->setStretchFactor(this->lab_startTip, 2);

}

void MIShow::setAnimation()
{
    this->labOpacity = new QGraphicsOpacityEffect(this->lab_startTip);
    // 创建两个动画，让它们顺序执行
    this->fadeOut = new QPropertyAnimation(this->labOpacity, "opacity");
    this->fadeIn = new QPropertyAnimation(this->labOpacity, "opacity");
    this->sequentialAnimation = new QSequentialAnimationGroup(this->lab_startTip);

    // 为label设置GraphicsEffect
    labOpacity->setOpacity(1);
    lab_startTip->setGraphicsEffect(labOpacity);

    // 逐渐消失动画
    this->fadeOut->setDuration(1000);
    this->fadeOut->setStartValue(1.0);
    this->fadeOut->setEndValue(0.0);
    this->fadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    // 逐渐浮现动画
    this->fadeIn->setDuration(1000);
    this->fadeIn->setStartValue(0.0);
    this->fadeIn->setEndValue(1.0);
    this->fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    // 为动画组添加动画
    this->sequentialAnimation->addAnimation(this->fadeOut);
    this->sequentialAnimation->addAnimation(this->fadeIn);
    this->sequentialAnimation->setLoopCount(-1);

    // 开始动画
    this->sequentialAnimation->start();


}

void MIShow::deleteAllItems()
{
    // 找到并删除所有label控件
    QList<QLabel*> listLabel = this->findChildren<QLabel*>();
    foreach (QLabel *label, listLabel) {
        delete label;
    }

    // 找到并删除所有layout控件
//    QList<QVBoxLayout*> listLayout = this->findChildren<QVBoxLayout*>();
//    foreach (QVBoxLayout *layout, listLayout) {
//        delete layout;
//    }

    this->installEventFilter(this);
    mainStage();
}

void MIShow::mainStage()
{
    lab_Cross = new QLabel(this);
    lab_Hand = new QLabel(this);
    lab_Arrow = new QLabel(this);

    lab_Cross->setText("＋");
    // 居中显示
    lab_Cross->resize(100, 50);
    lab_Cross->setAlignment(Qt::AlignCenter);
    lab_Cross->setStyleSheet("QLabel{font-size:68px;font:bold;color:red}");

    lab_Hand->setAlignment(Qt::AlignCenter);
    lab_Hand->adjustSize();

    lab_Arrow->setAlignment(Qt::AlignCenter);
    lab_Arrow->adjustSize();

    layout_V->addWidget(lab_Cross);
    layout_V->addWidget(lab_Hand);
    layout_V->addWidget(lab_Arrow);

    lab_Cross->show();
    lab_Hand->hide();
    lab_Arrow->hide();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MIShow::updateStates);

    timer->start(2000);

}

void MIShow::updateStates()
{
    switch(state){
        case 0:
            lab_Cross->hide();
            lab_Arrow->hide();
            lab_Hand->show();
            state = 1;

            randomValue = dist(rng);
            qDebug() << "randomValue::" << randomValue;
            timer->start(2000);

            if(randomValue){
                pixmap_Hand.load(":/MIPlay/imgs/left.png");
            }else{
                pixmap_Hand.load(":/MIPlay/imgs/right.png");
            }
            lab_Hand->setPixmap(pixmap_Hand);

            break;
        case 1:
            lab_Cross->hide();
            lab_Hand->hide();
            lab_Arrow->show();
            state = 2;

            timer->start(4000);
            emit markChange(randomValue);
            if(randomValue){
                pixmap_Arrow.load(":/MIPlay/imgs/left_arrow.png");
            }else{
                pixmap_Arrow.load(":/MIPlay/imgs/right_arrow.png");
            }
            lab_Arrow->setPixmap(pixmap_Arrow);

            break;
        case 2:
            lab_Cross->hide();
            lab_Hand->hide();
            lab_Arrow->hide();
            state = 3;
            timer->start(2000);
            break;
        case 3:
            lab_Cross->show();
            lab_Hand->hide();
            lab_Arrow->hide();
            state = 0;
            timer->start(2000);

            count++;
            controlwidget->setFinishNum(count);
            controlwidget->setRemainNum(test_num-count);
            break;
    }

    if(count == test_num){
        timer->stop();
    }
}
void MIShow::initControlWidget()
{
    controlwidget=new MIControlWidget;
    controlwidget->setAllNum(test_num);
    connect(controlwidget,&MIControlWidget::start,this,&MIShow::start);
    connect(controlwidget,&MIControlWidget::reset,this,&MIShow::reset);
    connect(controlwidget,&MIControlWidget::fullscreen,this,[=](bool status){
        if(status)
        {
            showFullScreen();
        }
        else
        {
            showNormal();
        }
    });
}
void MIShow::reset()
{
    QList<QLabel*> listLabel = this->findChildren<QLabel*>();
    foreach (QLabel *label, listLabel) {
        delete label;
    }
    QString str = "MI实验分为三个阶段：\n\n\
第一阶段：0-2s，休息准备期。在该阶段受试者可以休息，身体动作小，眨眼。\n\n\
第二阶段：2-4s，提示阶段。在该阶段，左手或右手移动动画会出现在屏幕上，以提醒受试者接下来的手部任务。\n\n\
第三阶段：4-8s，MI过程。在此期间，受试者根据监视器中的箭头提示，在相应方向上执行手移动的MI任务。\n\n";
    this->lab_description = new QLabel();
    this->lab_startTip = new QLabel();


    lab_description->setText(str);
    lab_description->setAlignment(Qt::AlignCenter);
    lab_description->setStyleSheet("QLabel{font-size:34px;font:bold}");

    this->lab_startTip->setText(">按空格键开始<\n");
    this->lab_startTip->setAlignment(Qt::AlignCenter);
    this->lab_startTip->setStyleSheet("QLabel{font-size:34px;font:bold}");

    // 设置动画
    setAnimation();

    layout_V->addWidget(this->lab_description);
    layout_V->addWidget(this->lab_startTip);
    layout_V->setStretchFactor(this->lab_description, 8);
    layout_V->setStretchFactor(this->lab_startTip, 2);
    isStart=false;
    count=0;
    emit resetSignal();
}

void MIShow::start()
{
    QString text=QInputDialog::getText(this,tr("登记"),tr("文件名"),QLineEdit::Normal);
    if(text.isEmpty())
    {
       QMessageBox::about(this,"消息提示","请输入文件名");
    }
    else
    {
        qDebug()<<"登记成功";
        setEnabled(true);
        emit filenameChenckIn(text);
    }
    if(!isStart){
        deleteAllItems();
        isStart = true;
    }
    emit startSignal();
}

void MIShow::show()
{
    QWidget::show();
    controlwidget->show();
}
