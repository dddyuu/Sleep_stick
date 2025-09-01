#ifndef PARSE_H
#define PARSE_H
#include "QThread"
#include "protocols.h"
#include "QMutex"
class Parse:public QThread
{
    Q_OBJECT
public:
    Parse();
    ~Parse();
    void start();
    void append(QByteArray);
    quint8 getChannelNum();
    void setDecodeStatus(bool status);
signals:
    void parseFinished(QList<double> data,QList<double>);
private:
    void run() override;
    QByteArray data;
    Protocols *protocols;
    bool status=false;
    QMutex mutex;
    bool  decode_status=false;
};

#endif // PARSE_H
