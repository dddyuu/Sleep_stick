#ifndef MISHOW_H
#define MISHOW_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QList>
#include <qdebug.h>
#include <QTimer>
#include <random>
#include <QPoint>
#include <QImage>
#include <QPixmap>
#include "micontrolwidget.h"
using namespace std;

namespace Ui {
class MIShow;
}

class MIShow : public QWidget
{
    Q_OBJECT

public:
    explicit MIShow(QWidget *parent = nullptr);
    ~MIShow();

    void initConnection();
    void init();
    void setAnimation();

    void deleteAllItems();
    void mainStage();

    void updateStates();

    void reset();
    void start();
    void show();

signals:
    //0:右手 1:左手
    void markChange(int);
    void startSignal();
    void resetSignal();
    void filenameChenckIn(QString);
private:
    Ui::MIShow *ui;
    QPropertyAnimation *propertyAnimation;
    MIControlWidget *controlwidget;
    void initControlWidget();
    int test_num=100;
    QLabel *lab_description;
    QLabel *lab_startTip;

    QLabel *lab_Cross;
    QLabel *lab_Hand;
    QLabel *lab_Arrow;

    QImage *img;

    QHBoxLayout *layout_H;
    QVBoxLayout *layout_V;

    QPropertyAnimation *fadeOut;
    QPropertyAnimation *fadeIn;
    // 移动动画
    QPropertyAnimation *handAnimation;

    // 透明度动画
    QGraphicsOpacityEffect *labOpacity;
    // 动画组
    QSequentialAnimationGroup *sequentialAnimation;

    QTimer *timer;

    int state = 0;  // 状态机
    int count = 0;  // 计数

    int randomValue = -1, randomValue2;

    bool isStart = false;
    // 伪随机数生成器
    mt19937 rng;
    // 0和1之间的均匀分布
    uniform_int_distribution<int> dist;

    QPixmap pixmap_Hand;
    QPixmap pixmap_Arrow;

signals:


        // Object Interface
protected:
    // 重写按键事件方法，实现按键ESC键关闭窗口
    void keyPressEvent(QKeyEvent *event) override{
        if(event->key() == Qt::Key_Escape){
            close();
        }else if(event->key() == Qt::Key_Space){
            if(!isStart){
                deleteAllItems();
                isStart = true;
            }
        }else{
            QWidget::keyPressEvent(event);  //交给其他按键事件
        }
    }

    // 重写事件过滤方法
    bool eventFilter(QObject *obj, QEvent *event) override{
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Space) {
                // 处理空格键的默认行为
                return false; // 返回false以便继续处理默认行为
            }
        }
        return QWidget::eventFilter(obj, event); // 调用基类的eventFilter处理其他事件
    }
};

#endif // MISHOW_H
