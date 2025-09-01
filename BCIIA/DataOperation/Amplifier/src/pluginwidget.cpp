#include "pluginwidget.h"
#include "ui_pluginwidget.h"
#include "QFileDialog"
#include "QDebug"
#include "QJsonArray"
#include "QJsonObject"
#include "QListWidgetItem"
PluginWidget::PluginWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginWidget)
{
    ui->setupUi(this);
    this->setWindowTitle("脑电采集插件");
}

PluginWidget::~PluginWidget()
{
    delete ui;
}
void PluginWidget::loadPluginInfo(QJsonArray info)
{
    //清除之前的插件库信息
    plugin_info.clear();
    if(ui->plugin_list->count()!=0)
    {
       ui->plugin_list->clear();
    }
    if(info.size()==0)
    {
        return;
    }
    //重新缓存插件库信息
    int plugin_num=info.size();
    for(int i=0;i<plugin_num;i++)
    {
        QString name=info[i].toObject().take("name").toString();
        QListWidgetItem *item=new QListWidgetItem();
        item->setText(name);
        ui->plugin_list->addItem(item);
        QString path=info[i].toObject().take("path").toString();
        QString describe=info[i].toObject().take("describe").toString();

        PluginInfo info;
        info.name=name;
        info.path=path;
        info.describe=describe;
        plugin_info.append(info);
    }
    ui->plugin_list->setCurrentRow(0);
}
void PluginWidget::on_remove_clicked()
{
    emit remove(ui->plugin_list->currentRow());
}
void PluginWidget::on_append_clicked()
{
    QString plugin_dir=QCoreApplication::applicationDirPath();
    QString plugin_path=QFileDialog::getOpenFileName(this,tr("添加插件"),plugin_dir,tr("(*.dll)"));
    if(!plugin_path.isEmpty())
    {
        emit append(plugin_path);
    }

}

void PluginWidget::on_apply_clicked()
{
    if(ui->plugin_list->count()==0)
    {
        return;
    }
    else
    {
        int index=ui->plugin_list->currentRow();
        this->hide();
        emit loadPlugin(plugin_info[index].path);
    }
}
void PluginWidget::on_plugin_list_itemClicked(QListWidgetItem *item)
{
    int current_index=ui->plugin_list->currentRow();
    QString str="name:"+plugin_info[current_index].name+"\n"+
            "describe:"+plugin_info[current_index].describe;
    ui->plugin_info->setText(str);
}
