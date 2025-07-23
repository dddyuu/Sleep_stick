#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "QButtonGroup"
#include "mediawidget.h"
#include "mishow.h"
namespace Ui {
class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
signals:
    void currentIndexChanged(int);
    void start();
    void stop();
    void labelsChanged(QVector<quint8>);
    void resetSignal();
    void filenameChenckIn(QString);
    void subjectRegistered();
    void videoEnd();
    void markChange(int);
private:
    Ui::MainWidget *ui;
    QButtonGroup *button_group;
    MediaWidget *mediawidget;
    MIShow *mishow;
    void init();
    void readLabels(QString);
};

#endif // MAINWIDGET_H
