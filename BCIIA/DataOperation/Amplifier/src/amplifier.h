#ifndef AMPLIFIER_H
#define AMPLIFIER_H

#include "QPluginLoader"
#include "amplifierdevice.h"
#include "QTimer"
#include "datathread.h"
#include "pluginmanage.h"
#include "pluginwidget.h"
class  Amplifier:public QObject
{
    Q_OBJECT
public:
    Amplifier();   
    ~Amplifier();
    void start();
    void pause();
    void stop();
    QStringList getChannelName();//获取通道数目
    QVariantList getChanlocs(); //获取导联信息
    QWidget* getConnectWidget();//获取设备连接窗口
    QWidget* getConfigWidget();//获取配置窗口
    quint16 getSampleRate();
    QList<uint8_t> getEEGIndex(); //获取全部脑电的索引
    QList<uint8_t> getMuscleIndex(); //获取肌电的索引
    QList<uint8_t> getBreathIndex(); //获取呼吸的索引
    QList<uint8_t> getHeartIndex(); //获取心电的索引
    void showConnectWidget();
    PluginWidget *getPluginWidget() const;
    void showPluginWidget();
    void setDecodeStatus(bool);
    void clearFilter();
    bool getStatus() const;

private slots:
    void loadPluginStatus(bool);

signals:
    void disconnected();
    void chartDataFinished(QList<double>);
    void preproDatafinished(QList<double>);
    void rawDataFinished(QList<QList<double>>);
    void locallabelFinished(QList<uint8_t>);
    void error(QString);
    void connected();
    void loadPluginSucceed();

private slots:
    void checkConnectStatus();
private:
    AmplifierDevice* amplifierdevice=NULL;
    //定时器
    QTimer *timer;
    void  initTimer();
    //数据线程
    DataThread *datathread;
    void initDataThread();
    //缓冲区管理
    QList<QList<double>> dataBuffer;
    int bufferSize = 100;
    void manageBuffer(const QList<QList<double>>& data);
    //插件管理
    PluginManage *pluginmanage;
    void initPluginManage();
    //插件管理页面
    PluginWidget *pluginwidget;
    void  initPluginWidget();

    //连接标志
    bool connect_status=false;

    //采集器状态
    bool status=false;
};

#endif // AMPLIFIER_H
