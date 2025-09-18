#include "filestorage.h"
#include "QFileInfo"
#include "QFileDialog"
#include "QDebug"
#include "QDateTime"
#include "storageconfig.h"
#include "matstorage.h"
#include "QTimer"
#include "QElapsedTimer"
#include <QMutex>

FileStorage* FileStorage::m_instance = nullptr;
QMutex mutex;
QWaitCondition condition;
bool dataReady = false;
bool labelReady = false;

FileStorage* FileStorage::instance(QObject* parent)
{
    static QMutex instanceMutex;
    QMutexLocker locker(&instanceMutex);
    if (!m_instance) {
        m_instance = new FileStorage(parent);
    }
    return m_instance;
}

FileStorage::FileStorage(QObject* parent) :QObject(parent)
{
    qRegisterMetaType<QList<double>>("QList<double>");
    qRegisterMetaType<QList<QList<double> >>("QList<QList<double>>");
    qRegisterMetaType<QList<QList<double>>>("QList<QList<double>>");
    init();
    setConnect();
}

FileStorage::~FileStorage()
{
    // 清理TCP连接
    if (tcpSocket) {
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            tcpSocket->disconnectFromHost();
            tcpSocket->waitForDisconnected(3000);
        }
        tcpSocket->deleteLater();
        tcpSocket = nullptr;
    }
    if (storage != nullptr)
    {
        delete storage;
        storage = nullptr;
    }
    if (elapsedTimer != nullptr)
    {
        delete elapsedTimer;
        elapsedTimer = nullptr;
    }
    m_instance = nullptr;
}

void FileStorage::append(QList<QList<double> > data)
{
    if (start_flag && !pause_flag)
    {
        //qDebug() << "Appending data with" << data.size() << "channels"<<cachingActive;
        // 只有在缓存激活时才进行TCP数据传输
        if (cachingActive) {
            // 将数据添加到缓存
            for (int i = 0; i < data.size(); i++) {
                if (cachedData.size() <= i) {
                    cachedData.append(QList<double>());
                }
                cachedData[i].append(data[i]);
            }

            // 如果TCP转发启用且连接正常，发送当前数据
            if (tcpForwardingEnabled && tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState) {
                //qDebug() << "Forwarding data to TCP server. Samples:" << (data.isEmpty() ? 0 : data[0].size());
                sendDataToTcp(data);
            }

            // 检查是否达到3分钟缓存时长
            quint64 cachedSamples = 0;
            if (!data.isEmpty()) {
                cachedSamples = totalSampleCount + data[0].size() - cacheStartSampleCount;
            }

            if (cachedSamples >= cacheTargetSamples) {
                // 缓存完成，通过信号发送缓存的数据
                emit cachedDataReady(cachedData);
                qDebug() << "3-minute caching completed. Total cached samples:" << cachedSamples;
                qDebug() << "Cached data emitted with" << cachedData.size() << "channels";

                // 清理缓存并重置状态
                cachedData.clear();
                cachingActive = false;

                // 保持TCP连接，不断开
                qDebug() << "TCP connection maintained after caching completed";
            }
        }
        //原始正常存储
        amplifer_data.append(data);
        // 更新总采样点计数
        if (!data.isEmpty()) {
            totalSampleCount += data[0].size(); // 假设所有通道的采样点数相同
        }
        dataReady = true;
        checkReady();
        if (amplifer_data.bufferFull())
        {
            save();
        }
    }
}

void FileStorage::appendLabel(quint8 label1)
{
    if (start_flag && !pause_flag && elapsedTimer->isValid())
    {
        // 创建标签信息
        LabelInfo labelInfo;
        labelInfo.label = label1;
        labelInfo.timestamp = elapsedTimer->elapsed(); // 获取毫秒时间戳
        labelInfo.samplePoint = totalSampleCount;      // 当前采样点

        labelInfoList.append(labelInfo);
        label.append(label1);

        // 输出调试信息
        qDebug() << "Label appended:"
            << "Label=" << label1
            << "Time=" << labelInfo.timestamp << "ms"
            << "Sample Point=" << labelInfo.samplePoint;

        labelReady = true;
        checkReady();
    }
}

void FileStorage::checkReady()
{
    if (dataReady && labelReady)
    {
        QMutexLocker locker(&mutex);
        condition.wakeOne();  // 唤醒保存操作
    }
}

void FileStorage::initTimer()
{
    timer = new QTimer();
    elapsedTimer = new QElapsedTimer();
    connect(timer, &QTimer::timeout, this, &FileStorage::save);
}

void FileStorage::start()
{
    QString filename = storage->getFilename();
    if (!filename.isEmpty())
    {
        qDebug() << "开始保存";
        if (stop_flag)
        {
            start_flag = true;
            pause_flag = false;
            stop_flag = false;
        }
        else
        {
            stop_flag = false;
            pause_flag = true;
            start_flag = true;
        }
        if (pause_flag)
        {
            start_flag = true;
            pause_flag = false;
        }

        // 启动计时器并重置计数器
        elapsedTimer->start();
        totalSampleCount = 0;
        labelInfoList.clear();

        qDebug() << "Timer started, sample rate:" << currentSampleRate;
    }
}

void FileStorage::pause()
{
    pause_flag = true;
    save();
}

void FileStorage::stop()
{
    if (mode == 0)
    {
        timer->stop();
    }
    start_flag = false;
    stop_flag = true;

    // 停止计时器
    if (elapsedTimer->isValid()) {
        qint64 totalTime = elapsedTimer->elapsed();
        qDebug() << "Recording stopped. Total time:" << totalTime << "ms";
        qDebug() << "Total samples recorded:" << totalSampleCount;
    }

    save();
    storage->stop();
}

void FileStorage::setChannelNum(quint8 value)
{
    storage->setChannelNum(value);
}

void FileStorage::setSrate(quint16 rate)
{
    currentSampleRate = rate;
    storage->setSrate(rate);
}

void FileStorage::setChanlocs(QVariantList value)
{
    storage->setChanlocs(value);
}

void FileStorage::setFileName(QString filenane)
{
    storage->setFilename(filenane);
}

void FileStorage::appendEvent(int type)
{
    qDebug() << "getevernt" << type << ":" << amplifer_data.getLen()[0] / 2;
    int len = amplifer_data.getLen()[0];
    storage->appendEvent(type, len / 2);

    // 更新当前事件类型
    currentEventType = type;

    // 特殊处理事件类型 51：开始 3 分钟数据缓存和TCP传输
    if (type == 51) {
        startCaching();

        // 确保TCP连接已建立
        if (tcpForwardingEnabled && !tcpServerAddress.isEmpty()) {
            if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState) {
                connectToTcpServer();
            }
            qDebug() << "Event 51: TCP data transmission activated";

            // 发送事件51通知
            sendEventNotification(51);
        }
    }
    // 特殊处理事件类型 54：停止TCP数据传输但保持连接
    else if (type == 54) {
        cachingActive = false;
        qDebug() << "Event 54: TCP data transmission stopped, connection maintained";
        save();
        // 发送事件54通知给Python
        sendEventNotification(54);
    }
}

void FileStorage::sendEventNotification(int eventType)
{
    if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "TCP socket not connected, cannot send event notification";
        return;
    }

    // 获取当前文件名
    QString currentFilename = storage->getFilename();

    // 构建事件通知数据包（无实际数据，仅事件信息）
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian); // 使用大端序

    // 写入事件通知包头信息
    stream << quint32(0x87654321);  // 事件通知包标识符（不同于数据包）
    stream << quint32(eventType);   // 事件类型
    stream << currentFilename;      // 当前文件名
    stream << quint64(totalSampleCount);  // 当前总采样点数

    // 添加时间戳
    qint64 timestamp = elapsedTimer ? elapsedTimer->elapsed() : 0;
    stream << qint64(timestamp);

    // 发送事件通知
    qint64 bytesWritten = tcpSocket->write(packet);
    tcpSocket->flush();

    bool success = (bytesWritten == packet.size());
    qDebug() << "Event notification sent:" << bytesWritten << "bytes, success:" << success;
    qDebug() << "Event type:" << eventType << "Filename:" << currentFilename;
    qDebug() << "Sample count:" << totalSampleCount << "Timestamp:" << timestamp << "ms";
}

void FileStorage::onTcpDataReceived()
{
    if (!tcpSocket) return;

    // 读取所有可用数据
    QByteArray data = tcpSocket->readAll();
    tcpReceiveBuffer.append(data);

    qDebug() << "Received" << data.size() << "bytes from Python";

    // 处理接收到的整数数据
    processPythonIntData();
}

//void FileStorage::processPythonIntData()
//{
//    // 整数数据处理：每4字节为一个整数
//    while (tcpReceiveBuffer.size() >= 4) {
//        // 使用大端序读取整数
//        QDataStream stream(tcpReceiveBuffer);
//        stream.setByteOrder(QDataStream::BigEndian);
//
//        qint32 receivedInt;
//        stream >> receivedInt;
//
//        // 验证接收到的整数是否在有效范围内 (0, 1, 2)
//        if (receivedInt >= 0 && receivedInt <= 2) {
//            qDebug() << "Received valid integer from Python:" << receivedInt;
//            emit pythonIntReceived(receivedInt);
//        }
//        else {
//            qDebug() << "Received invalid integer from Python:" << receivedInt;
//        }
//
//        // 移除已处理的4字节
//        tcpReceiveBuffer.remove(0, 4);
//    }
//}
void FileStorage::processPythonIntData()
{
    // 处理新的数据格式：4字节长度 + N字节数据
    while (tcpReceiveBuffer.size() >= 4) {
        // 首先读取长度
        QDataStream stream(tcpReceiveBuffer);
        stream.setByteOrder(QDataStream::BigEndian);

        qint32 length;
        stream >> length;

        // 检查长度是否合理（避免恶意数据）
        if (length < 0 || length > 1000) {
            qDebug() << "Received invalid length from Python:" << length;
            // 清空缓冲区避免持续错误
            tcpReceiveBuffer.clear();
            return;
        }

        // 检查是否有足够的数据
        if (tcpReceiveBuffer.size() < 4 + length) {
            // 数据还没接收完，等待更多数据
            break;
        }

        // 读取标签数据
        QList<quint8> labelList;

        // 跳过已读取的4字节长度
        tcpReceiveBuffer.remove(0, 4);

        for (int i = 0; i < length; i++) {
            if (tcpReceiveBuffer.size() > i) {
                quint8 labelValue = static_cast<quint8>(tcpReceiveBuffer.at(i));
                labelList.append(labelValue);
            }
        }

        // 移除已处理的数据字节
        tcpReceiveBuffer.remove(0, length);

        // 验证接收到的标签数据
        bool isValid = true;
        for (quint8 label : labelList) {
            if (label > 2) { // 假设有效标签范围是0-2
                isValid = false;
                break;
            }
        }

        if (isValid && !labelList.isEmpty()) {
            qDebug() << "Received valid label list from Python:" << labelList;

            // 处理接收到的标签列表
            for (quint8 label : labelList) {
                appendLabel(label); // 逐个添加标签
                emit pythonIntReceived(static_cast<int>(label)); // 发送信号
            }

            // 或者可以添加一个新的信号来处理整个列表
            // emit pythonLabelListReceived(labelList);
        }
        else {
            qDebug() << "Received invalid label list from Python:" << labelList;
        }
    }
}
void FileStorage::startCaching()
{
    // 计算 3 分钟需要的采样点数 (3分钟 * 60秒 * 采样率 * 通道数)
    quint64 cacheDurationSamples = 3 * 60 * currentSampleRate * amplifer_data.getSignalsChannelNum()[0];

    // 设置缓存标志和参数
    cachingActive = true;
    cacheStartSampleCount = totalSampleCount;
    cacheTargetSamples = cacheDurationSamples;

    // 清空之前的缓存数据
    cachedData.clear();

    qDebug() << "Event 51 received - Starting 3-minute data caching";
    qDebug() << "Cache duration samples:" << cacheDurationSamples;
    qDebug() << "Current sample count:" << totalSampleCount;
}

void FileStorage::startTcpConnection()
{
    if (tcpForwardingEnabled && !tcpServerAddress.isEmpty()) {
        connectToTcpServer();
        qDebug() << "TCP connection started manually to" << tcpServerAddress << ":" << tcpServerPort;
    }
    else {
        qDebug() << "Cannot start TCP connection: forwarding disabled or address empty";
    }
}

void FileStorage::init()
{
    this->start_flag = false;
    this->pause_flag = false;
    this->stop_flag = true;
    this->mode = 1;
    this->totalSampleCount = 0;
    this->currentSampleRate = 500; // 默认采样率500Hz
    this->currentEventType = 0;    // 初始化当前事件类型

    // 初始化缓存相关变量
    this->cachingActive = false;
    this->cacheStartSampleCount = 0;
    this->cacheTargetSamples = 0;
    this->cachedData.clear();

    // 初始化TCP相关变量
    this->tcpSocket = nullptr;
    this->tcpServerAddress = "127.0.0.1";  // 默认本地地址
    this->tcpServerPort = 8888;            // 默认端口
    this->tcpForwardingEnabled = true;     // 默认启用TCP转发
    this->tcpReceiveBuffer.clear();        // 初始化接收缓冲区

    storage = new MatStorage;
    initTimer();
    initTcpConnection();

    // 自动启动TCP连接
    startTcpConnection();

    // 连接Python整数接收信号 - 测试用
    connect(this, &FileStorage::pythonIntReceived, this, [this](int value) {
        qDebug() << "*** Processing Python integer:" << value << " ***";

        // 根据接收到的整数值进行不同处理
        switch (value) {
        case 0:
            qDebug() << "Python sent classification result: Class 0";
            break;
        case 1:
            qDebug() << "Python sent classification result: Class 1";
            break;
        case 2:
            qDebug() << "Python sent classification result: Class 2";
            break;
        default:
            qDebug() << "Unknown Python result:" << value;
            break;
        }
        });

    //初始化配置
    StorageConfig::init();
    initStorageConfigWidget();
    //获取文件保存目录
    this->dir = StorageConfig::getSavePath();
    quint16 storage_time = StorageConfig::getTime();
    //毫秒级精确度
    this->timer->setTimerType(Qt::PreciseTimer);
    this->timer->setInterval(storage_time * 1000);

    QList<quint8> channel_num = { 2 };
    QList<unsigned int> srate = { 500 }; // 修改为500Hz
    QStringList signals_name = { "eeg" };
    amplifer_data.setSignalsChannelNum(channel_num);
    amplifer_data.setSignalsName(signals_name);
    amplifer_data.setSignalsSrate(srate);
    amplifer_data.setTimeLen(600);
    amplifer_data.alloc_data();
}

void FileStorage::setConnect()
{
    setStorageConnect();
}

void FileStorage::setStorageConnect()
{
    connect(this, &FileStorage::stopSignal, storage, &Storage::stop, Qt::DirectConnection);
    connect(storage, &Storage::saveFinish, this, &FileStorage::saveFinish, Qt::DirectConnection);
    connect(storage, &Storage::mergeMsg, this, &FileStorage::mergeMsg, Qt::DirectConnection);
}

void FileStorage::save()
{
    qDebug() << "Saving" << this->label << "labels";
    qDebug() << "Label info count:" << labelInfoList.size();

    // 输出标签详细信息
    for (const LabelInfo& info : labelInfoList) {
        double timeInSeconds = info.timestamp / 1000.0;
        qDebug() << "Label:" << info.label
            << "Time:" << timeInSeconds << "s"
            << "Sample:" << info.samplePoint;
    }

    qDebug() << this->label;
    storage->save(amplifer_data.getData(), this->label, amplifer_data.getLen(),
        amplifer_data.getSignalsChannelNum(), amplifer_data.getSignalsName());
    amplifer_data.reset();

    // 重置就绪标志
    {
        QMutexLocker locker(&mutex);
        dataReady = false;
        labelReady = false;
    }
}

StorageConfigWidget* FileStorage::getStorageconfigwidget() const
{
    return storageconfigwidget;
}

void FileStorage::initStorageConfigWidget()
{
    storageconfigwidget = new StorageConfigWidget;
}

//tcp连接
void FileStorage::initTcpConnection()
{
    if (tcpSocket) {
        tcpSocket->deleteLater();
    }

    tcpSocket = new QTcpSocket(this);

    // 连接TCP信号槽
    connect(tcpSocket, &QTcpSocket::connected, this, &FileStorage::onTcpConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &FileStorage::onTcpDisconnected);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
        this, &FileStorage::onTcpError);
    //连接数据接收信号
    connect(tcpSocket, &QTcpSocket::readyRead, this, &FileStorage::onTcpDataReceived);
}

void FileStorage::setTcpServerAddress(const QString& address, quint16 port)
{
    tcpServerAddress = address;
    tcpServerPort = port;
    qDebug() << "TCP server address set to:" << address << ":" << port;
}

void FileStorage::enableTcpForwarding(bool enabled)
{
    tcpForwardingEnabled = enabled;
    qDebug() << "TCP forwarding" << (enabled ? "enabled" : "disabled");
}

void FileStorage::connectToTcpServer()
{
    if (!tcpSocket || tcpSocket->state() == QAbstractSocket::ConnectedState) {
        return;
    }

    if (tcpSocket->state() == QAbstractSocket::ConnectingState) {
        return; // 已经在连接中
    }

    qDebug() << "Connecting to TCP server:" << tcpServerAddress << ":" << tcpServerPort;
    tcpSocket->connectToHost(tcpServerAddress, tcpServerPort);
}

void FileStorage::onTcpConnected()
{
    qDebug() << "TCP connection established successfully";
}

void FileStorage::onTcpDisconnected()
{
    qDebug() << "TCP connection disconnected";
}

void FileStorage::onTcpError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "TCP connection error:" << socketError << tcpSocket->errorString();
}

void FileStorage::sendDataToTcp(const QList<QList<double>>& data)
{
    if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "TCP socket not connected, cannot send data";
        emit tcpDataSent(false);
        return;
    }

    if (data.isEmpty()) {
        qDebug() << "No data to send via TCP";
        emit tcpDataSent(false);
        return;
    }

    // 获取当前文件名
    QString currentFilename = storage->getFilename();

    // 构建包含数据、事件类型和文件名的数据包
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian); // 使用大端序

    // 写入数据包头信息
    stream << quint32(0x12345678);  // 数据包标识符
    stream << quint32(currentEventType);  // 当前事件类型
    stream << currentFilename;      // 当前文件名

    // 写入数据维度信息
    quint32 channelCount = data.size();
    quint32 sampleCount = data.isEmpty() ? 0 : data[0].size();
    stream << channelCount;
    stream << sampleCount;

    // 写入实际数据
    for (const QList<double>& channel : data) {
        for (double value : channel) {
            stream << value;
        }
    }

    // 发送数据
    qint64 bytesWritten = tcpSocket->write(packet);
    tcpSocket->flush();

    bool success = (bytesWritten == packet.size());


 /*   qDebug() << "TCP data sent:" << bytesWritten << "bytes, success:" << success;
    qDebug() << "Event type:" << currentEventType << "Filename:" << currentFilename;
    qDebug() << "Channels:" << channelCount << "Samples:" << sampleCount;*/
    emit tcpDataSent(success);
}