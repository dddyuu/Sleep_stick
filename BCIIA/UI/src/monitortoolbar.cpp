#include "monitortoolbar.h"
#include "QDebug"
MonitorToolBar::MonitorToolBar(QToolBar *parent)
    : QToolBar{parent}
{
    startAction.setIcon(QIcon(":/start.png"));
    stopAction.setIcon(QIcon(":/stop.png"));
    connectAction.setIcon(QIcon(":/connect.png"));
    decodeAndEncodeAction.setIcon(QIcon(":/decode.png"));
    //encodeAction.setIcon(QIcon(":/encode.png"));
    increaseChannelAction.setIcon(QIcon(":/IncreaseChannel.png"));
    increasedotAction.setIcon(QIcon(":/Increasedot.png"));
    increasescaleAction.setIcon(QIcon(":/Increasescale.png"));
    ReduceChannelAction.setIcon(QIcon(":/ReduceChannel.png"));
    ReducedotAction.setIcon(QIcon(":/Reducedot.png"));
    ReducescaleAction.setIcon(QIcon(":/Reducescale.png"));
    lastAction.setIcon(QIcon(":/last.png"));
    monitorAction.setIcon(QIcon(":/monitor.png"));
    nextAction.setIcon(QIcon(":/next.png"));
    pauseAction.setIcon(QIcon(":/pause.png"));
    pluginAction.setIcon(QIcon(":/plugin.png"));
    settingAction.setIcon(QIcon(":/setting.png"));
    // 设置禁止移动属性,工具栏默认贴在上方
    this->setFloatable(false);
    this->setMovable(false);

    // 工具栏添加菜单项
    this->addAction(&monitorAction);
    this->addAction(&startAction);
    this->addAction(&pauseAction);
    this->addAction(&stopAction);
    this->addAction(&lastAction);
    this->addAction(&nextAction);
    //this->addAction(&encodeAction);
    this->addAction(&increaseChannelAction);
    this->addAction(&ReduceChannelAction);
    this->addAction(&increasedotAction);
    this->addAction(&ReducedotAction);
    this->addAction(&increasescaleAction);
    this->addAction(&ReducescaleAction);
    this->addAction(&decodeAndEncodeAction);
    this->addAction(&connectAction);
    this->addAction(&pluginAction);
    this->addAction(&settingAction);
    //设置禁用
    this->disabledAction();
    //设置链接,点击action发送对应的信号
    this->initConnect();
}
void MonitorToolBar::disabledAction()
{
    startAction.setEnabled(false);
    stopAction.setEnabled(false);
    decodeAndEncodeAction.setEnabled(false);
    //encodeAction.setEnabled(false);
    increaseChannelAction.setEnabled(false);
    increasedotAction.setEnabled(false);
    increasescaleAction.setEnabled(false);
    lastAction.setEnabled(false);
    monitorAction.setEnabled(false);
    nextAction.setEnabled(false);
    pauseAction.setEnabled(false);
    //pluginAction.setEnabled(false);
    ReduceChannelAction.setEnabled(false);
    ReducedotAction.setEnabled(false);
    ReducescaleAction.setEnabled(false);
}

void MonitorToolBar::enableAction()
{
//    startAction.setEnabled(true);
    //stopAction.setEnabled(true);
    decodeAndEncodeAction.setEnabled(true);
    //encodeAction.setEnabled(true);
    increaseChannelAction.setEnabled(true);
    increasedotAction.setEnabled(true);
    increasescaleAction.setEnabled(true);
    lastAction.setEnabled(true);
    monitorAction.setEnabled(true);
    nextAction.setEnabled(true);
    //pauseAction.setEnabled(true);
    pluginAction.setEnabled(true);
    ReduceChannelAction.setEnabled(true);
    ReducedotAction.setEnabled(true);
    ReducescaleAction.setEnabled(true);
}
void MonitorToolBar::initConnect()
{
    connect(&connectAction,&QAction::triggered,this,[=](){
        qDebug()<<"链接";
        emit this->connectSignal();
        //点击链接按钮后启用其他按钮
    });

    connect(&startAction,&QAction::triggered,this,[=](){
        qDebug()<<"开始";
        this->pauseAction.setEnabled(true);
        this->stopAction.setEnabled(true);
        this->startAction.setEnabled(false);
        emit this->startSignal();
    });

    connect(&pauseAction,&QAction::triggered,this,[=](){
        qDebug()<<"暂停";
        emit this->pauseSignal();
    });

    connect(&stopAction,&QAction::triggered,this,[=](){
        qDebug()<<"结束";
        this->pauseAction.setEnabled(false);
        this->stopAction.setEnabled(false);
        this->startAction.setEnabled(true);
        emit this->stopSignal();
    });

    connect(&decodeAndEncodeAction,&QAction::triggered,this,[=](){
        if(decodeFlag)
        {
            qDebug()<<"解码";
            emit this->decodeSignal();
            decodeAndEncodeAction.setIcon(QIcon(":/encode.png"));
            decodeFlag = 0;
        }
        else
        {
            qDebug()<<"编码";
            emit this->encodeSignal();
            decodeAndEncodeAction.setIcon(QIcon(":/decode.png"));
            decodeFlag = 1;
        }
    });

//    connect(&encodeAction,&QAction::triggered,this,[=](){
//        qDebug()<<"编码";
//        emit this->encodeSignal();
//    });

    connect(&increaseChannelAction,&QAction::triggered,this,[=](){
        qDebug()<<"增加通道";
        emit this->increaseChannelSignal();
    });

    connect(&increasedotAction,&QAction::triggered,this,[=](){
        qDebug()<<"增加点数";
        emit this->increasedotSignal();
    });

    connect(&increasescaleAction,&QAction::triggered,this,[=](){
        qDebug()<<"放大曲线";
        emit this->increasescaleSignal();
    });

    connect(&ReduceChannelAction,&QAction::triggered,this,[=](){
        qDebug()<<"减少通道";
        emit this->ReduceChannelSignal();
    });

    connect(&ReducedotAction,&QAction::triggered,this,[=](){
        qDebug()<<"减少点数";
        emit this->ReducedotSignal();
    });

    connect(&ReducescaleAction,&QAction::triggered,this,[=](){
        qDebug()<<"缩小曲线";
        emit this->ReducescaleSignal();
    });

    connect(&lastAction,&QAction::triggered,this,[=](){
        qDebug()<<"上一个";
        emit this->lastSignal();
    });

    connect(&nextAction,&QAction::triggered,this,[=](){
        qDebug()<<"下一个";
        emit this->nextSignal();
    });

    connect(&monitorAction,&QAction::triggered,this,[=](){
        if(monitorFlag)
        {
            qDebug()<<"监视";
            emit this->monitorSignal();
            monitorAction.setIcon(QIcon(":/monitor2.png"));
            monitorFlag = 0;
            startAction.setEnabled(true);
        }
        else
        {
            qDebug()<<"停止监视";
            emit this->stopmonitorSignal();
            monitorAction.setIcon(QIcon(":/monitor.png"));
            monitorFlag = 1;
            startAction.setEnabled(false);
            stopAction.setEnabled(false);
            pauseAction.setEnabled(false);
        }
    });

    connect(&pluginAction,&QAction::triggered,this,[=](){
        qDebug()<<"设置";
        emit this->pluginSignal();
    });

    connect(&settingAction,&QAction::triggered,this,[=](){
        emit this->settingSignal();
    });
}
