#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <QWidget>
#include "taskflow.h"
namespace Ui {
class MediaWidget;
}

class MediaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MediaWidget(QWidget *parent = nullptr);
    ~MediaWidget();
    void show();
    void setVideoPath(QString);
private slots:
    void fullScreen(bool);
signals:
    void currentIndexChanged(int);
    void stop();
    void start();
    void resetSignal();
    void filenameChenckIn(QString);
    void subjectRegistered();
    void videoEnd();
private:
    Ui::MediaWidget *ui;
    //任务流程窗口
    TaskFlow *taskflow;
    void init();
    void setConnect();

};

#endif // MEDIAWIDGET_H
