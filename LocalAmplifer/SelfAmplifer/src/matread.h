#ifndef MATREAD_H
#define MATREAD_H

#include <QObject>
#include "QThreadPool"
#include "QCoreApplication"
#include "matio.h"
class MatRead : public QObject
{
    Q_OBJECT
public:
    explicit MatRead(QObject *parent = nullptr);
    void  setDir(QString dir);
    void  write();
    void  read();
    void  setDataDim(int channel_num,int point_num);
    QList<QList<double>> readlocalmat(QString dir);
    QList<uint8_t> readlocallabel(QString dir);
signals:

private:
    //文件的路径
    QString dir;
    //文件名
    QStringList filesname;
    //合并的文件名
    QString filename;
    void  writeFilename();
    //mat数据
    matvar_t *eeg;
    double getexpand = 1;
    //脑电数据
    double *eegdata;
    //脑电数据当尾地址
    double *eegdata_end;
    //脑电数据的维度
    size_t dim[2];
    void  readData(matvar_t *eeg);

    //脑电导联信息
    matvar_t *chanlocs=NULL;
    void  readLabel(matvar_t *eeg);

    //事件信息
    QList<matvar_t *> events;
    void writeEvent();
    void readevent(matvar_t *eeg);

    //原始事件信息
    QList<matvar_t *> urevents;
    void writeUrevent();
    void readUrevent(matvar_t *eeg);

    //采样率
    matvar_t  *srate=NULL;
    void readSrate(matvar_t *eeg);

};

#endif // MATREAD_H
