#ifndef MATFILESTORAGE_H
#define MATFILESTORAGE_H
#include "QObject"
#include "mat.h"
#include "storage.h"
#include "mat.h"
#include "QProcess"
class MatStorage:public Storage
{
    Q_OBJECT
public:
    MatStorage();
    ~MatStorage();
    void setChannelLabels(QString lables)override;
    void setChannelNum(quint8 num)override;
public slots:
    void save(double*,int num)override;
    void save(double*,int num,double*,int);
    void save(QList<double*>, QList<quint8>, QList<quint64>, QList<quint8>, QStringList);
    void stop() override;
private slots:
    void matProcessFinished(int,QProcess::ExitStatus);
private:
    MAT mat;
    QString getCurrentFileName();
    int sample_id=0;
    //通道数
    quint8 channel_num;
    //获取保存的目录
    QString getFilePath();
    QProcess *mat_process;
    void initMatProcess();
    void setMatProcessConnect();
};

#endif // MATSTORAGE_H
