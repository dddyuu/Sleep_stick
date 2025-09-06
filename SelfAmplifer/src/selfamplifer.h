#ifndef SELFAMPLIFER_H
#define SELFAMPLIFER_H

#include <QObject>
#include "amplifierdevice.h"
#include "eegdata.h"
#include "MainWindow.h"        // 引入MainWindow
#include "Bluetooth/DataParser.h"        // 引入数据解析器

class SelfAmplifer : public AmplifierDevice
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID "SelfAmplifer" FILE "selfamplifer.json")
        Q_INTERFACES(AmplifierDevice)

public:
    SelfAmplifer();
    ~SelfAmplifer();

    // AmplifierDevice接口实现
    void start() override;                                    // 开始采集
    void stop() override;                                     // 停止采集
    QStringList getChannelName() override;                    // 获取通道标签
    quint8 getChannnelNum() override;                        // 获取通道数目
    quint16 getSampleRate() override;                        // 获取采样率
    QWidget* getConnectWidget() override;                    // 获取设备连接窗口
    QWidget* getConfigWidget() override;                     // 获取配置窗口
    QList<double> getOneData() override;                     // 获取一帧数据
    QList<QList<double>> getChartData() override;           // 获取一组数据
    QList<QList<double>> getRawData() override;             // 获取一组数据
    QList<uint8_t> getEEGGameIndex() override;              // 获取游戏脑电的索引
    QList<uint8_t> getEEGIndex() override;                  // 获取全部脑电的索引
    QList<uint8_t> getEyeIndex() override;                  // 获取眼电的索引
    QList<uint8_t> getMuscleIndex() override;               // 获取肌电信号的索引
    QList<uint8_t> getBreathIndex() override;               // 获取呼吸信号的索引
    QList<uint8_t> getHeartIndex() override;                // 获取心电信号的索引
    bool connectStatus() override;                           // 连接状态
    void setDecodeStatus(bool status) override;             // 设置解码状态

private slots:
    // 蓝牙事件处理槽函数
    void onBluetoothDataReceived(const DataParser::ParsedData& data);
    void onConnectionStateChanged(BluetoothDeviceController::ConnectionState state);
    void onBluetoothError(const QString& error);

private:
    // MainWindow界面（包含蓝牙功能）
    MainWindow* mainWindow;

    // 数据缓存
    EEGData eegdata;

    // 初始化函数
    void init();

    // 采集器通道数
    quint8 channel_num;

    // 采集器状态
    bool status;

    // 解码状态
    bool decodeStatus;
};

#endif // SELFAMPLIFER_H