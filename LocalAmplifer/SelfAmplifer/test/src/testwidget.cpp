#include "testwidget.h"
#include "ui_testwidget.h"
testWidget::testWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::testWidget)
{
    ui->setupUi(this);
    amplifier.start();
}

testWidget::~testWidget()
{
    delete ui;
}

void testWidget::timerEvent(QTimerEvent *event)
{
    qDebug()<<amplifier.getArrayData();
}

void testWidget::on_pushButton_3_clicked()
{
    amplifier.getConnectWidget()->show();
}

void testWidget::on_pushButton_clicked()
{
    amplifier.start();
}

void testWidget::on_pushButton_2_clicked()
{
    amplifier.stop();
}

void testWidget::on_pushButton_4_clicked()
{
    QList<QList<double>> data=amplifier.getArrayData();
    for(int i=0;i<data.size();i++)
    {
        for(int j=0;j<16;j++)
        {
            data[i][j]=filter.B_filter(j,data[i][j]);
        }
    }
    startTimer(1000);
}
