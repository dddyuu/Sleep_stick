#include "filestorage.h"
#include "QFileInfo"
#include "QFileDialog"
#include "QDebug"
#include "QDateTime"
#include "storageconfig.h"
#include "matstorage.h"
#include "QTimer"
#include <QMutex>

FileStorage* FileStorage::m_instance = nullptr;
QMutex mutex;
QWaitCondition condition;
bool dataReady = false;
bool labelReady = false;
FileStorage* FileStorage::instance(QObject *parent)
{
    static QMutex instanceMutex;
    QMutexLocker locker(&instanceMutex);
    if (!m_instance) {
        m_instance = new FileStorage(parent);
    }
    return m_instance;
}
FileStorage::FileStorage(QObject *parent):QObject(parent)
{
    qRegisterMetaType<QList<double>>("QList<double>");
    qRegisterMetaType<QList<QList<double> >>("QList<QList<double>>");
    init();
    setConnect();
}

FileStorage::~FileStorage()
{
    if(storage != nullptr)
    {
        delete storage;
        storage = nullptr;
    }
    m_instance = nullptr;
}
void FileStorage::append(QList<QList<double> > data)
{
    // qDebug()<<"appendLabel"<<start_flag<<" "<<pause_flag;
    if (start_flag && !pause_flag)
    {
        amplifer_data.append(data);
        dataReady = true;
        checkReady();
        if (amplifer_data.bufferFull())
        {
            //qDebug() << "保存数据";
            save();
        }
    }
}

void FileStorage::appendLabel(quint8 label1){
    // qDebug()<<"appendLabel"<<start_flag<<" "<<pause_flag;
    // if (start_flag && !pause_flag)
    // {
    label.append(label1);
    // qDebug()<<"appendLabel"<<label;
        // amplifer_data.appendlabel(label);
    labelReady = true;
    checkReady();
    // }
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
    timer=new QTimer();
    connect(timer,&QTimer::timeout,this,&FileStorage::save);
}
void FileStorage::start()
{
    QString filename = storage->getFilename();
    if(!filename.isEmpty())
    {
        qDebug() << "开始保存";
        if(stop_flag)
        {
            start_flag=true;
            pause_flag=false;
            stop_flag=false;
        }
        else
        {
            stop_flag=false;
            pause_flag=true;
            start_flag=true;
        }
        if(pause_flag)
        {
            start_flag=true;
            pause_flag=false;
        }
    }

}
void FileStorage::pause()
{
    pause_flag=true;
    save();
}
void FileStorage::stop()
{
    if(mode==0)
    {
        timer->stop();
    }
    start_flag=false;
    stop_flag=true;
    save();
    storage->stop();
}
void FileStorage::setChannelNum(quint8 value)
{
    storage->setChannelNum(value);
}

void FileStorage::setSrate(quint16 rate)
{
    storage->setSrate(rate);
}
void FileStorage::setChanlocs(QVariantList value)
{
    storage->setChanlocs(value);
}

void FileStorage::setFileName(QString filenane)
{
    //QFileInfo fileinfo(filenane);
    //QString dir=fileinfo.absoluteFilePath();
    storage->setFilename(filenane);
}
void FileStorage::appendEvent(int type)
{
    storage->appendEvent(type,amplifer_data.getLen()[0]);
}
void FileStorage::init()
{
    this->start_flag=false;
    this->pause_flag=false;
    this->stop_flag=true;
    this->mode=1;
    storage=new MatStorage;
    initTimer();
    //初始化配置
    StorageConfig::init();
    initStorageConfigWidget();
    //获取文件保存目录
    this->dir=StorageConfig::getSavePath();
    quint16 storage_time=StorageConfig::getTime();
    //毫秒级精确度
    this->timer->setTimerType(Qt::PreciseTimer);
    this->timer->setInterval(storage_time*1000);

    QList<quint8> channel_num = {32};
    QList<unsigned int> srate = {1000};
    QStringList signals_name = { "eeg"};
    amplifer_data.setSignalsChannelNum(channel_num);
    amplifer_data.setSignalsName(signals_name);
    amplifer_data.setSignalsSrate(srate);
    amplifer_data.setTimeLen(120);
    amplifer_data.alloc_data();
}
void FileStorage::setConnect()
{
    setStorageConnect();
}
void FileStorage::setStorageConnect()
{
    connect(this,&FileStorage::stopSignal,storage,&Storage::stop,Qt::DirectConnection);
    connect(storage,&Storage::saveFinish,this,&FileStorage::saveFinish,Qt::DirectConnection);
    connect(storage,&Storage::mergeMsg,this,&FileStorage::mergeMsg,Qt::DirectConnection);
}

// void FileStorage::save()
// {
//     storage->save(amplifer_data.getData(),amplifer_data.getlabel(),amplifer_data.getLen(), amplifer_data.getSignalsChannelNum(), amplifer_data.getSignalsName());
//     amplifer_data.reset();
// }
void FileStorage::save()
{
    // 在save中
    qDebug() << "Saving" << this->label << "labels";
    // {
    //     QMutexLocker locker(&mutex);
    //     while (!dataReady || !labelReady)
    //     {
    //         condition.wait(&mutex);  // 等待数据和标签准备好
    //     }
    //     qDebug()<<"save is already;"<<dataReady<<labelReady;
    // }
    qDebug()<<this->label;
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
StorageConfigWidget *FileStorage::getStorageconfigwidget() const
{
    return storageconfigwidget;
}

void FileStorage::initStorageConfigWidget()
{
    storageconfigwidget=new StorageConfigWidget;
}
