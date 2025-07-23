#include "storageconfig.h"
#include "QDir"
#include "QCoreApplication"
#include "QDebug"
StorageConfig::StorageConfig(QObject *parent) : QObject(parent)
{

}

void StorageConfig::init()
{

    QString path="config/BCIMoniotr/FileStroage/config.ini";
    if(QFileInfo::exists(path))
    {
        return;
    }
    QSettings  settings(path,QSettings::IniFormat);
    QString save_path=QCoreApplication::applicationDirPath()+"/eegdata";
    settings.setValue("path",save_path);
    settings.setValue("time",2*60);
    settings.setValue("connectGameStatus",true);
}
QString StorageConfig::getSavePath()
{
    QString path="config/BCIMoniotr/FileStroage/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("path"))
    {
        return settings.value("path").toString();
    }
    else
    {
        return "";
    }
}

int StorageConfig::getSaveNum()
{
    QString path="config/BCIMoniotr/FileStroage/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("num"))
    {
        return settings.value("num").toInt();
    }
    else
    {
        return 20000;
    }
}

int StorageConfig::getTime()
{
    QString path="config/BCIMoniotr/FileStroage/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("time"))
    {
        return settings.value("time").toInt();
    }
    else
    {
        return 2*60;
    }
}
void StorageConfig::setSavePath(QString save_path)
{
    QString path="config/BCIMoniotr/FileStroage/config.ini";
    QSettings  settings(path,QSettings::IniFormat);
    settings.setValue("path",save_path);
}

bool StorageConfig::getConnectGameStatus()
{
    QString path="config/BCIMoniotr/FileStroage/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("connectGameStatus"))
    {
        return settings.value("connectGameStatus").toBool();
    }
    else
    {
        return false;
    }
}

int StorageConfig::getTrainTime(QString path)
{
    QDir dir(path);
    QStringList files=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    return files.size();
}
