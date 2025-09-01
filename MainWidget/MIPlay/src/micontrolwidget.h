#ifndef MICONTROLWIDGET_H
#define MICONTROLWIDGET_H

#include <QWidget>

namespace Ui {
class MIControlWidget;
}

class MIControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MIControlWidget(QWidget *parent = nullptr);
    ~MIControlWidget();
    void setAllNum(int);
    void setFinishNum(int);
    void setRemainNum(int);

signals:
    void start();
    void reset();
    void fullscreen(bool);
private slots:
    void on_start_clicked();

    void on_reset_clicked();

    void on_full_screen_clicked();

private:
    Ui::MIControlWidget *ui;
    bool screen_status=false;
};

#endif // MICONTROLWIDGET_H
