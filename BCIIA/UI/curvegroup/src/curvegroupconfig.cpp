#include "curvegroupconfig.h"
#include "QFileInfo"
void CurveGroupConfig::init()
{

    QString path="config/BCIMoniotr/CurveGroup/config.ini";
    if(QFileInfo::exists(path))
    {
        return;
    }
    QSettings  settings(path,QSettings::IniFormat);
    settings.setValue("groupnum",8);
    settings.setValue("maxpoint",1000);
}

uint8_t CurveGroupConfig::getGroupNum()
{
    QString path="config/BCIMoniotr/CurveGroup/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("groupnum"))
    {
        return settings.value("groupnum").toUInt();
    }
    else
    {
        return 6;
    }
}

uint16_t CurveGroupConfig::getMaxPoint()
{
    QString path="config/BCIMoniotr/CurveGroup/config.ini";
    QSettings  settings(path,QSettings::IniFormat);

    if(settings.contains("maxpoint"))
    {
        return settings.value("maxpoint").toUInt();
    }
    else
    {
        return 1000;
    }
}
