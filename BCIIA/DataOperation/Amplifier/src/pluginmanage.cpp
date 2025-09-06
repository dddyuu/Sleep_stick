#include "pluginmanage.h"
#include "QDir"
#include "QCoreApplication"
#include "QJsonDocument"
#include "QJsonObject"
#include "QDebug"
#include "QPluginLoader"
#include "QMessageBox"
PluginManage::PluginManage(AmplifierDevice** amplifierdevice)
{
    this->amplifierdevice=amplifierdevice;
}
void PluginManage::append(QString path)
{
      QPluginLoader loader(path);
      QJsonObject info=loader.metaData().take("MetaData").toObject();

      QString name=info.take("name").toString();
      QString describe=info.take("describe").toString();

      QJsonObject object;
      object["name"]=name;
      object["path"]=path;
      object["describe"]=describe;
      array.append(object);
      QString dir=QCoreApplication::applicationDirPath()+"/plugin";
      QString pulgin_config_path=dir+"/config.json";
      QFile file(pulgin_config_path);
      file.open(QIODevice::WriteOnly);
      QByteArray json=QJsonDocument(array).toJson();
      file.write(json);

      emit pluginInfoChanged(array);

}

void PluginManage::remove(int index)
{
     array.removeAt(index);
     QString dir=QCoreApplication::applicationDirPath()+"/plugin";
     QString pulgin_config_path=dir+"/config.json";
     QFile file(pulgin_config_path);
     file.open(QIODevice::WriteOnly);
     QByteArray json=QJsonDocument(array).toJson();
     file.write(json);
     emit pluginInfoChanged(array);
}
void PluginManage::loadPluginInfo()
{
    QDir  dir;
    QString path=QCoreApplication::applicationDirPath()+"/plugin";
    if(!dir.exists(path))
    {
        dir.mkpath(path);
        qDebug()<<"创建文件";
    }
    QString pulgin_config_path=path+"/config.json";
    QFile file(pulgin_config_path);
    if(!file.exists())
    {
        file.open(QIODevice::WriteOnly);
    }
    else
    {
        file.open(QIODevice::ReadOnly);
        QByteArray file_data=file.readAll();
        QJsonDocument doc=QJsonDocument::fromJson(file_data);
        array=doc.array();
    }
    emit pluginInfoChanged(array);
}

void PluginManage::loadPlugin(QString path)
{
    QPluginLoader loader(path);
    QObject *plugin=loader.instance();
    qDebug()<<loader.errorString();
    if(plugin){

        QJsonObject info=loader.metaData().take("MetaData").toObject();
        QString name=info.take("name").toString();
        if(this->name!=name)
        {
            //插件加载成功后，将之前的采集器释放掉
            if((*amplifierdevice)!=NULL)
            {
                bool isok=(*amplifierdevice)->connectStatus();
                if(isok)
                {
                    QMessageBox::about(NULL,"消息提示","请先断开设备");
                    return;
                }
                else
                {
                    delete *amplifierdevice;
                    *amplifierdevice=NULL;
                }

            }
            *amplifierdevice=qobject_cast<AmplifierDevice*>(plugin);

            if(*amplifierdevice)
            {
                this->name=name;
				qDebug() << "加载插件成功" << name;
                emit error(false);
            }
            else
            {

                emit error(true);;
            }
        }
        else
        {
            QMessageBox::about(NULL,"消息提示","请不要加载重复插件");
        }

    }
    else
    {
        emit error(true);;
    }
}
