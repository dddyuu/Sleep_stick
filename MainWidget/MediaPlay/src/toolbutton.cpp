#include "toolbutton.h"
#include "ui_toolbutton.h"

ToolButton::ToolButton(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToolButton)
{
    ui->setupUi(this);
}

ToolButton::~ToolButton()
{
    delete ui;
}

void ToolButton::setPerviousEnable(bool status)
{
    ui->pervious->setEnabled(status);
}

void ToolButton::setNextEnable(bool status)
{
    ui->next->setEnabled(status);
}

void ToolButton::pauseSlot()
{
    play_status=false;
    ui->play->setIcon(QIcon(":/mediaplay/image/play.png"));
}

void ToolButton::setPostion(quint8 value)
{
    ui->play_progress->setValue(value);
}

void ToolButton::on_play_clicked()
{
    if(play_status)
    {
        play_status=false;
        ui->play->setIcon(QIcon(":/mediaplay/image/play.png"));
        emit pause();
    }
    else
    {
        play_status=true;
        ui->play->setIcon(QIcon(":/mediaplay/image/pause.png"));
        emit start();
    }
}

void ToolButton::on_next_clicked()
{
    emit next();
}

void ToolButton::on_pervious_clicked()
{
    emit pervious();
}
void ToolButton::on_full_screen_clicked()
{
    if(full_status)
    {
        full_status=false;
    }
    else
    {
        full_status=true;
    }
    emit fullScreen(full_status);
}
