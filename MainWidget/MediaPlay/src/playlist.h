#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QWidget>

namespace Ui {
class PlayList;
}

class PlayList : public QWidget
{
    Q_OBJECT

public:
    explicit PlayList(QWidget *parent = nullptr);
    ~PlayList();
    void setPlayList(QStringList);
    void setCurrentRow(int);
signals:

private:
    Ui::PlayList *ui;
};

#endif // PLAYLIST_H
