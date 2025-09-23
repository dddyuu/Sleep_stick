#ifndef FILESTORAGE_H
#define FILESTORAGE_H
#include "QString"
#include "QThread"
#include "storageconfigwidget.h"
#include "storage.h"
#include "storagedata.h"
#include "matstorage.h"
#include "qwaitcondition.h"
#include "QElapsedTimer"
#include "QDateTime"
#include "QFileInfo"
#include <QTcpSocket>  // 添加TCP支持
#include <QHostAddress>

// 标签信息结构体
struct LabelInfo {
    quint8 label;           // 标签值
    qint64 timestamp;       // 时间戳（毫秒）
    quint64 samplePoint;    // 对应的采样点
};

class FileStorage :public QObject
{
    Q_OBJECT

public:
    // 单例访问接口
    static FileStorage* instance(QObject* parent = nullptr);

    // 删除拷贝构造函数和赋值运算符
    FileStorage(const FileStorage&) = delete;
    FileStorage& operator=(const FileStorage&) = delete;

    ~FileStorage();

    // 新建文件
    void creatFile();
    void creatFile(QString);

    // 插入事件
    void appendEvent(int);

    StorageConfigWidget* getStorageconfigwidget() const;

    // TCP相关接口
    void setTcpServerAddress(const QString& address, quint16 port);
    void enableTcpForwarding(bool enabled);
    void startTcpConnection();  // 手动启动TCP连接

public slots:
    //开始
    void start();
    //暂停
    void pause();
    //结束
    void stop();
    //设置通道数
    void setChannelNum(quint8 value);
    //设置采样率
    void setSrate(quint16 srate);
    //设置脑电导联分布信息
    void setChanlocs(QVariantList);
    //设置文件信息(账户和游戏)
    void setFileName(QString);
    //数据插入
    void append(QList<QList<double>> data);

    void appendLabel(quint8 label);
    void checkReady();

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(QAbstractSocket::SocketError socketError);
    void onTcpDataReceived();  // 新增：接收Python数据

private:
    // 私有构造函数确保单例
    explicit FileStorage(QObject* parent = nullptr);

    // 单例指针
    static FileStorage* m_instance;
    QList<quint8>label;
    QList<LabelInfo> labelInfoList;  // 存储标签信息列表
    //暂停标志
    bool  pause_flag;
    //开始标志
    bool  start_flag;
    //结束标志
    bool  stop_flag;
    //存储过程
    Storage* storage;
    // MatStorage *matstorage;
    //放大器数据
    File::Data amplifer_data;

    QTimer* timer;
    QElapsedTimer* elapsedTimer;  // 计时器
    quint64 totalSampleCount;     // 总采样点计数
    quint16 currentSampleRate;    // 当前采样率
    int currentEventType;         // 当前事件类型

    // 事件51缓存相关成员变量
    bool cachingActive;           // 是否正在进行缓存
    quint64 cacheStartSampleCount;// 缓存开始时的采样点计数
    quint64 cacheTargetSamples;   // 缓存目标采样点数（3分钟）
    QList<QList<double>> cachedData; // 缓存的数据

    // TCP相关成员变量
    QTcpSocket* tcpSocket;        // TCP客户端套接字
    QString tcpServerAddress;     // TCP服务器地址
    quint16 tcpServerPort;        // TCP服务器端口
    bool tcpForwardingEnabled;    // TCP转发是否启用
    QByteArray tcpReceiveBuffer;  // 接收缓冲区
    quint64 tcpSentSampleCount;
    void initTimer();
    void init();
    void setConnect();
    void setStorageConnect();
    void save();

    // 缓存相关函数
    void startCaching();          // 开始缓存
    void initTcpConnection();     // 初始化TCP连接
    void sendDataToTcp(const QList<QList<double>>& data);  // 发送数据到TCP服务器
    void connectToTcpServer();    // 连接到TCP服务器
    void sendEventNotification(int eventType);  // 发送事件通知
    void processPythonIntData();  // 处理Python发送的整数数据

    //保存模式，0：定时器保存，1：采样率保存，默认1
    unsigned short int mode;

    //配置窗口
    StorageConfigWidget* storageconfigwidget;
    void initStorageConfigWidget();

    //文件保存位置 
    QString dir;
signals:
    void storageSignal(double*, int num);
    void setNameSignal(QString);
    void stopSignal();
    void saveFinish(QString);
    void mergeMsg(QString);
    void cachedDataReady(QList<QList<double>> cachedData); // 缓存数据就绪信号
    void tcpDataSent(bool success); // TCP数据发送结果信号
    void pythonIntReceived(int value); // Python整数接收信号，实时计算标签
};
#endif // FILESTORAGE_H