#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QObject>
#include "QVariant"
class Communication : public QObject
{
    Q_OBJECT
public:
    virtual ~Communication(){};
    virtual bool connectstatus()=0;
    virtual QWidget *getConnectWidget()=0;
    virtual QByteArray  readData()=0;
    virtual void setParam(QString, QVariant) {};
signals:
    void newLogging(QString);
    void readyRead();
    void connected();
};

#endif // COMMUNICATION_H
