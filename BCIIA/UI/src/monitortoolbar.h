#pragma execution_character_set("utf-8")
#ifndef MONITORTOOLBAR_H
#define MONITORTOOLBAR_H

#include <QToolBar>
#include <QMenuBar>
#include <QAction>
class MonitorToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit MonitorToolBar(QToolBar *parent = nullptr);
    void disabledAction();
    void enableAction();
private:
    QMenuBar bar;
    QAction startAction;
    QAction stopAction;
    QAction connectAction;
    QAction decodeAndEncodeAction;
    int decodeFlag = 1;
    //QAction encodeAction;
    QAction increaseChannelAction;
    QAction increasedotAction;
    QAction increasescaleAction;
    QAction lastAction;
    QAction monitorAction;
    int monitorFlag = 1;
    QAction nextAction;
    QAction pauseAction;
    QAction pluginAction;
    QAction ReduceChannelAction;
    QAction ReducedotAction;
    QAction ReducescaleAction;

    QAction settingAction;//设置
    void initConnect();
signals:
    void connectSignal();
    void startSignal();
    void stopSignal();
    void decodeSignal();
    void encodeSignal();
    void increaseChannelSignal();
    void increasedotSignal();
    void increasescaleSignal();
    void lastSignal();
    void monitorSignal();
    void nextSignal();
    void pauseSignal();
    void pluginSignal();
    void ReduceChannelSignal();
    void ReducedotSignal();
    void ReducescaleSignal();
    void stopmonitorSignal();
    void settingSignal();
};

#endif // MONITORTOOLBAR_H
