#include "playlist.h"
#include "ui_playlist.h"
#include "QDir"
PlayList::PlayList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayList)
{
    ui->setupUi(this);
}

PlayList::~PlayList()
{
    delete ui;
}

void PlayList::setPlayList(QStringList paths)
{
    ui->videolist->addItems(paths);
    ui->videolist->setCurrentRow(0);
}

void PlayList::setCurrentRow(int row)
{
    ui->videolist->setCurrentRow(row);
}
