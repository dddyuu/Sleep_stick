#ifndef DATADOWN_H
#define DATADOWN_H

#include <QObject>
#include "QThread"
#include "QMutex"
class DataDown : public QThread
{
    Q_OBJECT
public:
    explicit DataDown(QObject *parent = nullptr);
    ~DataDown();
    void setChannelNum(quint8 num);
    void setSrate(const quint16 &value);
    void run() override;
public slots:
    void receiveData(QList<double>);
signals:
    void downData(QList<double>);
private:
    QVector<QList<double>> data;
    quint16 downsample;
    quint8 channel_num;
    bool status;
    QMutex mutex;


};

#endif // DATADOWN_H
