#ifndef PLUGINMANAGE_H
#define PLUGINMANAGE_H

#include <QObject>
#include "QJsonArray"
#include "amplifierdevice.h"
class PluginManage : public QObject
{
    Q_OBJECT
public:
    explicit PluginManage(AmplifierDevice**);
    void append(QString);
    void remove(int);
    //初始化加载插件信息,插件名字和插件信息
    void loadPluginInfo();
    //加载插件
    void loadPlugin(QString);
signals:
    void pluginInfoChanged(QJsonArray);
    void error(bool);
private:
    QJsonArray array;
    AmplifierDevice **amplifierdevice;
    QString name;

};

#endif // PLUGINMANAGE_H
