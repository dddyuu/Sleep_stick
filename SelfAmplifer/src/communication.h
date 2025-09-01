#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QObject>

class Communication : public QObject
{
    Q_OBJECT
public:
    virtual ~Communication(){};
    virtual bool connectstatus()=0;
    virtual QWidget *getConnectWidget()=0;
signals:
    void newLogging(QString);
    void readyRead(QByteArray);
    void connected();
};

#endif // COMMUNICATION_H
