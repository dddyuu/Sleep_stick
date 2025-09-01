#ifndef CONNECTWIDGET_H
#define CONNECTWIDGET_H

#include <QWidget>
#include "QMap"
#include "QVariant"
namespace Ui {
class ConnectWidget;
}
class ConnectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectWidget(QWidget *parent = nullptr);
    ~ConnectWidget();
public slots:
    void setConnectStatus(QString);

private slots:
    //void on_search_clicked();
    void on_chooseFileButton_clicked();
signals:
    void connectDevice(QMap<QString,QVariant>);
    void fileSelected(const QString& fileName);
private:
    Ui::ConnectWidget *ui;
    void init();
};

#endif // CONNECTWIDGET_H
