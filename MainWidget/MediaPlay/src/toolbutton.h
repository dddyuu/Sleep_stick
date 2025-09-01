#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include <QWidget>
namespace Ui {
class ToolButton;
}

class ToolButton : public QWidget
{
    Q_OBJECT

public:
    explicit ToolButton(QWidget *parent = nullptr);
    ~ToolButton();
    void setPerviousEnable(bool);
    void setNextEnable(bool);
public slots:
    void setPostion(quint8);
    void pauseSlot();
private slots:

    void on_play_clicked();

    void on_next_clicked();
    void on_pervious_clicked();

    void on_full_screen_clicked();

signals:
    void start();
    void pause();
    void pervious();
    void next();
    void fullScreen(bool);
private:
    Ui::ToolButton *ui;
    bool play_status=false;
    bool full_status=false;
};

#endif // TOOLBUTTON_H
