#ifndef FILESTORAGE_H
#define FILESTORAGE_H
#include "QString"
#include "QThread"
#include "storageconfigwidget.h"
#include "storage.h"
#include "storagedata.h"
#include "matstorage.h"
#include "qwaitcondition.h"
class FileStorage:public QObject
{
    Q_OBJECT

public:
    // 单例访问接口
    static FileStorage* instance(QObject *parent = nullptr);

    // 删除拷贝构造函数和赋值运算符
    FileStorage(const FileStorage&) = delete;
    FileStorage& operator=(const FileStorage&) = delete;

    ~FileStorage();

    // 新建文件
    void creatFile();
    void creatFile(QString);

    // 插入事件
    void appendEvent(int);

    StorageConfigWidget *getStorageconfigwidget() const;
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
private:
    // 私有构造函数确保单例
    explicit FileStorage(QObject *parent = nullptr);

    // 单例指针
    static FileStorage* m_instance;
    QList<quint8>label;
    //暂停标志
    bool  pause_flag;
    //开始标志
    bool  start_flag;
    //结束标志
    bool  stop_flag;
    //存储过程
    Storage *storage;
    // MatStorage *matstorage;
    //放大器数据
    File::Data amplifer_data;

    QTimer *timer;
    void initTimer();
    void init();
    void setConnect();
    void setStorageConnect();
    void save();
    //保存模式，0：定时器保存，1：采样率保存，默认1
    unsigned short int mode;

    //配置窗口
    StorageConfigWidget *storageconfigwidget;
    void initStorageConfigWidget();

    //文件保存位置 
    QString dir;
signals:
    void storageSignal(double*,int num);
    void setNameSignal(QString);
    void stopSignal();
    void saveFinish(QString);
    void mergeMsg(QString);
};
#endif // FILESTORAGE_H
