#include "selfamplifer.h"
#include "QDebug"

SelfAmplifer::SelfAmplifer()
{
    qRegisterMetaType<QList<double>>("QList<double>");
    qRegisterMetaType<DataParser::ParsedData>("DataParser::ParsedData");
    init();
}

SelfAmplifer::~SelfAmplifer()
{
    if (mainWindow) {
        mainWindow->deleteLater();
    }
}

void SelfAmplifer::start()
{
    status = true;
    qDebug() << "SelfAmplifer: 开始数据采集";
}

void SelfAmplifer::stop()
{
    status = false;
    qDebug() << "SelfAmplifer: 停止数据采集";
}

QStringList SelfAmplifer::getChannelName()
{
    return { "FP1", "FP2" };
}

quint8 SelfAmplifer::getChannnelNum()
{
    return 2;
}

quint16 SelfAmplifer::getSampleRate()
{
    return 1000;
}

QWidget* SelfAmplifer::getConnectWidget()
{
    return mainWindow;
}

QWidget* SelfAmplifer::getConfigWidget()
{
    return nullptr;
}

QList<double> SelfAmplifer::getOneData()
{
    QList<double> data;
    return data;
}

QList<QList<double>> SelfAmplifer::getChartData()
{
    return eegdata.getChartData(2);
}

QList<QList<double>> SelfAmplifer::getRawData()
{
    QList<QList<double>> data;
    data.append(eegdata.getRawData());
    return data;
}

QList<uint8_t> SelfAmplifer::getEEGGameIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getEEGIndex()
{
    return { 0, 1 };
}

QList<uint8_t> SelfAmplifer::getEyeIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getMuscleIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getBreathIndex()
{
    return {};
}

QList<uint8_t> SelfAmplifer::getHeartIndex()
{
    return {};
}

bool SelfAmplifer::connectStatus()
{
    if (mainWindow && mainWindow->getBluetoothController()) {
        return mainWindow->getBluetoothController()->connectionState() == BluetoothDeviceController::Connected;
    }
    return false;
}

void SelfAmplifer::setDecodeStatus(bool status)
{
    decodeStatus = status;
    qDebug() << "设置解码状态:" << status;
}

void SelfAmplifer::init()
{
    channel_num = 2; // FP1和FP2两个通道
    status = false;  // 初始化采集器状态
    decodeStatus = true; // 默认启用解码

    // 创建MainWindow作为连接界面
    mainWindow = new MainWindow();

    // 连接MainWindow中蓝牙控制器的数据接收信号
    if (mainWindow->getBluetoothController()) {
        connect(mainWindow->getBluetoothController(), &BluetoothDeviceController::dataReceived,
            this, &SelfAmplifer::onBluetoothDataReceived);

        connect(mainWindow->getBluetoothController(), &BluetoothDeviceController::connectionStateChanged,
            this, &SelfAmplifer::onConnectionStateChanged);

        connect(mainWindow->getBluetoothController(), &BluetoothDeviceController::errorOccurred,
            this, &SelfAmplifer::onBluetoothError);
    }

    qDebug() << "SelfAmplifer 初始化完成";
}

void SelfAmplifer::onBluetoothDataReceived(const DataParser::ParsedData& data)
{
    if (!status || !decodeStatus) {
        return; // 如果未开始采集或解码被禁用，则忽略数据
    }

    // 处理接收到的蓝牙数据
    if (data.valid && data.parsed_eeg_data.fp1.size() > 0 && data.parsed_eeg_data.fp2.size() > 0) {
        // 将两个通道的数据按顺序合并到一个QList中
        // 数据格式：[FP1_sample1, FP2_sample1, FP1_sample2, FP2_sample2, ...]

        int sampleCount = qMin(data.parsed_eeg_data.fp1.size(), data.parsed_eeg_data.fp2.size());

        for (int i = 0; i < sampleCount; ++i) {
            QList<double> combinedData;

            // 将FP1和FP2数据按顺序添加到组合数据中
            combinedData.append(static_cast<double>(data.parsed_eeg_data.fp1[i]));
            combinedData.append(static_cast<double>(data.parsed_eeg_data.fp2[i]));

            // 对于chart_data和raw_data，我们使用相同的数据
            // 如果需要不同的处理，可以在这里进行修改
            eegdata.append(combinedData, combinedData);
        }

        // qDebug() << QString("处理EEG数据 - 样本数: %1, 电池: %2%")
            // .arg(sampleCount)
            // .arg(data.battery);
    }
}

void SelfAmplifer::onConnectionStateChanged(BluetoothDeviceController::ConnectionState state)
{
    QString stateText;
    switch (state) {
    case BluetoothDeviceController::Disconnected:
        stateText = "未连接";
        break;
    case BluetoothDeviceController::Scanning:
        stateText = "扫描中";
        break;
    case BluetoothDeviceController::Connecting:
        stateText = "连接中";
        break;
    case BluetoothDeviceController::Connected:
        stateText = "已连接";
        break;
    }

    qDebug() << "蓝牙连接状态变更:" << stateText;
}

void SelfAmplifer::onBluetoothError(const QString& error)
{
    qDebug() << "蓝牙错误:" << error;
}
