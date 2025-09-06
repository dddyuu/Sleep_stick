#ifndef DATATHREAD_H
#define DATATHREAD_H

#include <QThread>
#include "amplifierdevice.h"
#include "filter.h"

class DataThread : public QThread
{
    Q_OBJECT
public:
    explicit DataThread(QObject *parent = nullptr);
    void run() override;
    void setDeivce(AmplifierDevice *);
    void setStatus(bool value);
    void destroy();
    bool getStatus() const;
    void clearFilter();
signals:
    void chartDataFinish(QList<double>);
    void rawDataFinished(QList<QList<double>>);
    void preproDatafinished(QList<double>);
    //void rawDataFinished(QList<double>);
    void locallabelFinished(QList<uint8_t>);
private:
    bool status;
    bool destroyFlag;
    AmplifierDevice *device;
    Filter filter;
};

#endif // DATATHREAD_H
