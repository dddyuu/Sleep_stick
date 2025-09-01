#include "mediaplay.h"
#include "QDir"
#include "QCollator"
MediaPlay::MediaPlay(QWidget *parent) : QWidget(parent)
{
    initPlayer();
    initLayout();
}

void MediaPlay::start()
{
    player->play();

}

void MediaPlay::pause()
{
    player->pause();
}

void MediaPlay::stop()
{
    player->stop();
}

void MediaPlay::next()
{
    playlist.next();
}

void MediaPlay::reset()
{
    playlist.setCurrentIndex(0);
    player->setPosition(0);
    player->pause();
    emit videoFinished();
}

void MediaPlay::previous()
{
    playlist.previous();
}

void MediaPlay::setSource(QString path)
{
    player->setMedia(QUrl(path));
}

void MediaPlay::setSource(QStringList videos)
{
    playlist.clear();
    for(auto video:videos)
    {
        playlist.addMedia(QUrl::fromLocalFile(video));
    }
    player->setPlaylist(&playlist);
}

void MediaPlay::setVideoPath(QString Path)
{
    this->path=Path;
    readVideosPath();
}

void MediaPlay::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if(status==QMediaPlayer::EndOfMedia)
    {
        emit videoEnd();
    }
}

void MediaPlay::positionChangedSlot(qint64 position)
{
     qint64 video_length=player->duration();
     if(video_length)
     {
         qint8 value=position*100/video_length;
         emit positionChanged(value);
     }

}

void MediaPlay::initPlayer()
{
    videowidget=new QVideoWidget();
    player=new QMediaPlayer();
    player->setVideoOutput(videowidget);
    //视频的播放进度
    connect(player,&QMediaPlayer::positionChanged,this,&MediaPlay::positionChangedSlot);
    //视频的状态变换
    connect(player,&QMediaPlayer::mediaStatusChanged,this,&MediaPlay::mediaStatusChanged);
    connect(&playlist,&QMediaPlaylist::currentIndexChanged,this,[=](int index){

       player->stop();
       emit currentIndexChanged(index);
       emit videoFinished();
    });
}

void MediaPlay::initLayout()
{
    widget_layout=new QVBoxLayout();
    widget_layout->addWidget(videowidget);
    this->setLayout(widget_layout);
}

QStringList MediaPlay::getVideosName() const
{
    return videosname;
}

void MediaPlay::readVideosPath()
{
//    QDir dir("D:\\project\\Qt\\CognitivePlatform\\seed-stimulation");
//    QDir dir("videos");
    QDir dir(path);
    QStringList paths=dir.entryList(QDir::Files);
    //文件排序
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(paths.begin(), paths.end(),
    [& collator](const QString & str1, const QString & str2)
    {
    return collator.compare(str1, str2) < 0;
    });
    QStringList video_path;
    //获取文件地址
    for(int i=0;i<paths.size();i++)
    {
        if(paths[i].contains("mp4"))
        {
            QFileInfo info(paths[i]);
            QString filename=info.baseName();
            videosname.append(filename);
            video_path.append(dir.path()+"/"+paths[i]);
        }

    }
    //设置视频地址
    setSource(video_path);
}
