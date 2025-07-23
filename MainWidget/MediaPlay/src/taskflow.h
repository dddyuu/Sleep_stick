#ifndef TASKFLOW_H
#define TASKFLOW_H

#include <QWidget>

namespace Ui {
class TaskFlow;
}

class TaskFlow : public QWidget
{
    Q_OBJECT

public:
    explicit TaskFlow(QWidget *parent = nullptr);
    ~TaskFlow();
    void setPlayList(QStringList);

public slots:
    void setPostion(quint8);
    void setCurrentIndex(int);
    void stop();
signals:
    void start();
    void pause();
    void pervious();
    void next();
    void fullScreen(bool);
    void resetSignal();
    void filenameChenckIn(QString);
    void subjectRegistered();
private slots:

    void on_reset_clicked();

    void on_check_in_clicked();

private:
    Ui::TaskFlow *ui;
    void setConnect();
    void setEnabled(bool enable);
};

#endif // TASKFLOW_H
