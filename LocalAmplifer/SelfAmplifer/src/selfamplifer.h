#ifndef SELFAMPLIFER_H
#define SELFAMPLIFER_H

#include "QObject"
#include "amplifierdevice.h"
#include "communicationmanage.h"
#include "communication.h"
#include "connectwidget.h"
#include "eegdata.h"
#include "parse.h"
#include "matio.h"
#include "matread.h"
class  SelfAmplifer:public AmplifierDevice
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "SelfAmplifer" FILE "selfamplifer.json")
    Q_INTERFACES(AmplifierDevice)
public:
    SelfAmplifer();
    ~SelfAmplifer();
    void start();//开始采集
    void stop();//停止采集
    QStringList getChannelName();//获取通道标签
    quint8 getChannnelNum(); //获取通道数目
    quint16 getSampleRate(); //获取采样率
    QWidget* getConnectWidget();//获取设备连接窗口
    QWidget* getConfigWidget();//获取配置窗口
    QList<double> getOneData();//获取一帧数据
    QList<QList<double>> getChartData();//获取一组数据
    QList<QList<double>> getRawData();//获取一组数据
    QList<uint8_t> getEEGGameIndex(); //获取游戏脑电的索引
    QList<uint8_t> getEEGIndex(); //获取全部脑电的索引
    QList<uint8_t> getEyeIndex(); //获取眼电的索引
    QList<uint8_t> getMuscleIndex(); //获取肌电信号的索引
    QList<uint8_t> getBreathIndex(); //获取呼吸信号的索引
    QList<uint8_t> getHeartIndex(); //获取心电信号的索引
    bool connectStatus();
    void setDecodeStatus(bool status) override;
    void setAmplifierParam(QString, QVariant);
    void incrementB() { bias++; }
private:
    //通信
    CommunicationManage manage;
    Communication *communication;
    //解析
    Parse parse;
    //缓存
    EEGData eegdata;
    void init();
    MatRead* matread;
    ConnectWidget* connectwidget;
    //采集器通道
    quint8 channel_num;
    //采集器状态
    bool status;
    int getB() const;
    void getLocalData(QString Dir);
    void initConnetWidget();
    int bias=0;
    int getpoint = 5;
    QList<QList<double>> Raw_data;
    QList<uint8_t> Label_data;
    bool connect_status;//

};

#endif // SELFAMPLIFER_H
