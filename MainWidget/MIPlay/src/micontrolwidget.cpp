#include "micontrolwidget.h"
#include "ui_micontrolwidget.h"

MIControlWidget::MIControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MIControlWidget)
{
    ui->setupUi(this);
    ui->reset->setEnabled(false);
}

MIControlWidget::~MIControlWidget()
{
    delete ui;
}

void MIControlWidget::setAllNum(int value)
{
    ui->all_num->setText(QString::number(value));
}

void MIControlWidget::setFinishNum(int value)
{
    ui->finish_num->setText(QString::number(value));
}

void MIControlWidget::setRemainNum(int value)
{
    ui->remain_num->setText(QString::number(value));
}

void MIControlWidget::on_start_clicked()
{
    emit start();
    ui->start->setEnabled(false);
    ui->reset->setEnabled(true);
}

void MIControlWidget::on_reset_clicked()
{
    emit reset();
    ui->start->setEnabled(true);
    ui->reset->setEnabled(false);
}

void MIControlWidget::on_full_screen_clicked()
{
    screen_status=!screen_status;
    emit fullscreen(screen_status);
}
