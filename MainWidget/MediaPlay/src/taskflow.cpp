#include "taskflow.h"
#include "ui_taskflow.h"
#include "QInputDialog"
#include "QMessageBox"
#include "QDebug"
TaskFlow::TaskFlow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskFlow)
{
    ui->setupUi(this);
    setConnect();
    setEnabled(false);
}

TaskFlow::~TaskFlow()
{
    delete ui;
}

void TaskFlow::setPlayList(QStringList name)
{
    ui->play_list->setPlayList(name);
}

void TaskFlow::setPostion(quint8 value)
{
    ui->btn_play->setPostion(value);
}

void TaskFlow::setCurrentIndex(int index)
{
    ui->play_list->setCurrentRow(index);
}

void TaskFlow::stop()
{
    ui->btn_play->pauseSlot();
}

void TaskFlow::setConnect()
{
    connect(ui->btn_play,&ToolButton::start,this,&TaskFlow::start);
    connect(ui->btn_play,&ToolButton::pause,this,&TaskFlow::pause);
    connect(ui->btn_play,&ToolButton::pervious,this,&TaskFlow::pervious);
    connect(ui->btn_play,&ToolButton::next,this,&TaskFlow::next);
    connect(ui->btn_play,&ToolButton::fullScreen,this,&TaskFlow::fullScreen);

}

void TaskFlow::setEnabled(bool enable)
{
    ui->btn_play->setEnabled(enable);
    ui->play_list->setEnabled(enable);
}

void TaskFlow::on_reset_clicked()
{
    setEnabled(false);
    emit resetSignal();
}

void TaskFlow::on_check_in_clicked()
{
    QString text=QInputDialog::getText(this,tr("登记"),tr("文件名"),QLineEdit::Normal);
    if(text.isEmpty())
    {
       QMessageBox::about(this,"消息提示","请输入文件名");
    }
    else
    {
        qDebug()<<"登记成功";
        setEnabled(true);
        emit filenameChenckIn(text);
        emit subjectRegistered();
    }
}
