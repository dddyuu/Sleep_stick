#ifndef INDEXWIDGET_H
#define INDEXWIDGET_H

#include <QWidget>
#include "QTabBar"
#include "QVBoxLayout"
#include "QStackedWidget"
//#include "aaa.h"
QT_BEGIN_NAMESPACE
namespace Ui { class IndexWidget; }
QT_END_NAMESPACE

class IndexWidget : public QWidget
{
    Q_OBJECT

public:
    IndexWidget(QWidget *parent = nullptr);
    ~IndexWidget();
    void addWidget(QString name,QWidget *widget);
private:
    Ui::IndexWidget *ui;

public slots:
    void tabClickSlot(int index);


private slots:
    void on_min_clicked();

    void on_max_clicked();

    void on_close_clicked();



signals:
    void  minSingal();
    void  maxSingal();
    void  closeSingal();

private:
    QTabBar *tabBar;
    QStackedWidget *widget;
    void init();
    void setStyle();
    void minMax();
    int lastIndex;
};
#endif // INDEXWIDGET_H
