#ifndef PLUGINWIDGET_H
#define PLUGINWIDGET_H

#include <QDialog>
#include "QListWidgetItem"
namespace Ui {
class PluginWidget;
}

class PluginWidget : public QDialog
{
    Q_OBJECT

public:
    explicit PluginWidget(QWidget *parent = nullptr);
    ~PluginWidget();
    void  loadPluginInfo(QJsonArray);
signals:
    void append(QString);
    void remove(int);
    void loadPlugin(QString);
private slots:
    void on_remove_clicked();

    void on_append_clicked();

    void on_apply_clicked();

    void on_plugin_list_itemClicked(QListWidgetItem *item);

private:
    typedef struct plugininfo
    {
        QString name;
        QString path;
        QString describe;
    }PluginInfo;
    Ui::PluginWidget *ui;
    QList<PluginInfo> plugin_info;

};

#endif // PLUGINWIDGET_H
