#ifndef COMMUNICATIONMANAGE_H
#define COMMUNICATIONMANAGE_H

#include <QObject>
#include "communication.h"
class CommunicationManage : public QObject
{
    Q_OBJECT
public:
    explicit CommunicationManage(QObject *parent = nullptr);
    ~CommunicationManage();
    Communication *setType(quint8);
signals:

private:
    Communication *communication=NULL;
};

#endif // COMMUNICATIONMANAGE_H
