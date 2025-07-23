#include "mediawidget.h"
#include "ui_mediawidget.h"

MediaWidget::MediaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MediaWidget)
{
    ui->setupUi(this);
    init();
    setConnect();
}

MediaWidget::~MediaWidget()
{
    delete ui;
}

void MediaWidget::show()
{
    QWidget::show();
    taskflow->show();
}

void MediaWidget::setVideoPath(QString path)
{
    ui->player->setVideoPath(path);
    taskflow->setPlayList(ui->player->getVideosName());
    ui->playlist->setPlayList(ui->player->getVideosName());
}

void MediaWidget::fullScreen(bool status)
{
    if(status)
    {
//        ui->playlist->hide();
        this->showFullScreen();
    }
    else
    {
//        ui->playlist->show();
        this->showNormal();
    }
}

void MediaWidget::init()
{
    taskflow=new TaskFlow(this);
    taskflow->setWindowFlag(Qt::Dialog);
    ui->playlist->hide();
    ui->play_button->hide();
}

void MediaWidget::setConnect()
{
    //播放器信号
    connect(ui->player,&MediaPlay::positionChanged,ui->play_button,&ToolButton::setPostion);
    connect(ui->player,&MediaPlay::positionChanged,taskflow,&TaskFlow::setPostion);
    connect(ui->player,&MediaPlay::currentIndexChanged,ui->playlist,&PlayList::setCurrentRow);
    connect(ui->player,&MediaPlay::currentIndexChanged,this,&MediaWidget::currentIndexChanged);
    connect(ui->player,&MediaPlay::currentIndexChanged,taskflow,&TaskFlow::setCurrentIndex);
    connect(ui->player,&MediaPlay::videoFinished,ui->play_button,&ToolButton::pauseSlot);
    connect(ui->player,&MediaPlay::videoFinished,taskflow,&TaskFlow::stop);
    connect(ui->player,&MediaPlay::videoFinished,this,&MediaWidget::stop);
    connect(ui->player,&MediaPlay::videoEnd,this,&MediaWidget::videoEnd);
    //播放按钮信号
    connect(ui->play_button,&ToolButton::start,ui->player,&MediaPlay::start);
    connect(ui->play_button,&ToolButton::start,this,&MediaWidget::start);
    connect(ui->play_button,&ToolButton::pause,ui->player,&MediaPlay::pause);
    connect(ui->play_button,&ToolButton::pervious,ui->player,&MediaPlay::previous);
    connect(ui->play_button,&ToolButton::next,ui->player,&MediaPlay::next);
    connect(ui->play_button,&ToolButton::fullScreen,this,&MediaWidget::fullScreen);
    //任务流程信号
    connect(taskflow,&TaskFlow::start,ui->player,&MediaPlay::start);
    connect(taskflow,&TaskFlow::start,this,&MediaWidget::start);
    connect(taskflow,&TaskFlow::pause,ui->player,&MediaPlay::pause);
    connect(taskflow,&TaskFlow::pervious,ui->player,&MediaPlay::previous);
    connect(taskflow,&TaskFlow::next,ui->player,&MediaPlay::next);
    connect(taskflow,&TaskFlow::fullScreen,this,&MediaWidget::fullScreen);
    connect(taskflow,&TaskFlow::resetSignal,this,&MediaWidget::resetSignal);
    connect(taskflow,&TaskFlow::resetSignal,ui->player,&MediaPlay::reset);
    connect(taskflow,&TaskFlow::filenameChenckIn,this,&MediaWidget::filenameChenckIn);
    connect(taskflow,&TaskFlow::subjectRegistered,this,&MediaWidget::subjectRegistered);

}
