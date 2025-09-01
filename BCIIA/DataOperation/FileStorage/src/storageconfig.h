#ifndef STORAGECONFIG_H
#define STORAGECONFIG_H

#include <QObject>
#include "QSettings"
class StorageConfig : public QObject
{
    Q_OBJECT
public:
    explicit StorageConfig(QObject *parent = nullptr); 
    static void init();
    static QString getSavePath();
    static int getSaveNum();
    static int getTime();
    static void setSavePath(QString);
    static bool getConnectGameStatus();
    static int getTrainTime(QString);
};

#endif // STORAGECONFIG_H
