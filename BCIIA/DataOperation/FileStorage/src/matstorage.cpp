#include "matstorage.h"

MatStorage::MatStorage()
{
    initMatProcess();
    setMatProcessConnect();
}

MatStorage::~MatStorage()
{

}
void MatStorage::setChannelLabels(QString lables)
{

}
void MatStorage::setChannelNum(quint8 num)
{
    channel_num=num;
}
void MatStorage::save(double *data, int num)
{

    //采样点累加
    Storage::save(data,num);
    //开始保存分段的数据
    QString filename=getCurrentFileName();
    mat.setFileName(filename);
    //保存采样率
    mat.saveSrate(getSrate());
    //保存脑电导联信息
    mat.saveChanlocs(getChanlocs());
    //保存事件信息
    mat.saveEevent(getEvent());
    mat.saveUrevent(getUrevent());
    mat.stop();


    //清除事件
    clearEvent();
    emit saveFinish(filename);
}

void MatStorage::save(double *data, int num, double *heart, int heart_num)
{
    //采样点累加
    Storage::save(data,num,heart,heart_num);
    //开始保存分段的数据
    QString filename=getCurrentFileName();
    mat.setFileName(filename);
    //保存采样率
    mat.saveSrate(getSrate());
    //保存脑电导联信息
    mat.saveChanlocs(getChanlocs());
    //保存事件信息
    mat.saveEevent(getEvent());
    mat.saveUrevent(getUrevent());
    mat.stop();
    //清除事件
    clearEvent();
    emit saveFinish(filename);
}
void MatStorage::save(QList<double*> data, QList<quint8> label, QList<quint64> len, QList<quint8> channel_num, QStringList signals_name)
{
    //开始保存分段的数据
    QString filename = getCurrentFileName();
    mat.setFileName(filename);
    mat.start(signals_name);
    mat.saveFileName(filename);

    mat.save(data,label,len,channel_num,signals_name);
    //保存采样率
    mat.saveSrate(getSrate());
    //保存脑电导联信息
    mat.saveChanlocs(getChanlocs());
    //保存事件信息
    mat.saveEevent(getEvent());
    mat.saveUrevent(getUrevent());
    mat.stop();
    //清除事件
    clearEvent();
    emit saveFinish(filename);
}
void MatStorage::stop()
{
    //将分段数据合成
    QString path=getFilePath();
    QString channel_num=QString::number(this->channel_num);
    QString eeg_num=QString::number(getPoint_num());
    QStringList argv={path,channel_num,eeg_num};
    qDebug()<<argv;
    mat_process->start("mat.exe",argv);
    emit mergeMsg("数据合并中");
    mat_process->waitForFinished();
    // qDebug()<<mat_process->readAll();
    // qDebug()<<"数据合并中";
//    重置采样数目
    Storage::stop();
}

void MatStorage::matProcessFinished(int, QProcess::ExitStatus)
{
    emit mergeMsg("合并结束");
}

QString MatStorage::getCurrentFileName()
{
    quint16 number;
    getNumber(number);
    sample_id=number;
    QString filename=getFilename();
    QFileInfo info(filename);
    QString dir=info.absolutePath();
    QString baseName=info.baseName();
    QString path=dir+"/"+baseName+"_"+QString::number(number)+".mat";
    return path;
}
QString MatStorage::getFilePath()
{
    QString filename=getFilename();
    QFileInfo info(filename);
    QString path=info.absolutePath();
    return path;
}

void MatStorage::initMatProcess()
{
    mat_process=new QProcess;
}

void MatStorage::setMatProcessConnect()
{
    connect(mat_process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(matProcessFinished(int,QProcess::ExitStatus)));
}
