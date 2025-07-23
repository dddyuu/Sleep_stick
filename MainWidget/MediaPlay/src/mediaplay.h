#ifndef MEDIAPLAY_H
#define MEDIAPLAY_H

#include <QWidget>
#include "QMediaPlayer"
#include "QVideoWidget"
#include "QVBoxLayout"
#include "QMediaPlaylist"
class MediaPlay : public QWidget
{
    Q_OBJECT
public:
    explicit MediaPlay(QWidget *parent = nullptr);
    void start();
    void pause();
    void stop();
    void next();
    void reset();
    void previous();
    void setSource(QString);
    void setSource(QStringList);
    void setVideoPath(QString Path);
    QStringList getVideosName() const;

private slots:
    void mediaStatusChanged(QMediaPlayer::MediaStatus);
    void positionChangedSlot(qint64);
signals:
    void positionChanged(quint8);
    //视频结束信号
    void videoFinished();
    //视频结束信号，用于打mark
    void videoEnd();
    void currentIndexChanged(int);
private:
    //窗口布局
    QVBoxLayout *widget_layout;
    //播放器窗口
    QVideoWidget *videowidget;
    //播放器
    QMediaPlayer *player;
    //播放列表
    QMediaPlaylist playlist;
    void initPlayer();
    void initLayout();
    //视频路径
    QStringList videosname;
    //读取视频的路径
    void readVideosPath();
    //视频路径
    QString path;

};

#endif // MEDIAPLAY_H
