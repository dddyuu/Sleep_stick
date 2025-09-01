#include "connectwidget.h"
#include "ui_connectwidget.h"
//#include "QSerialPortInfo"
#include "QMessageBox"
#include <QPushButton>
#include <QFileDialog>
ConnectWidget::ConnectWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConnectWidget)
{
    ui->setupUi(this);
    init();
}

ConnectWidget::~ConnectWidget()
{
    delete ui;
}
void ConnectWidget::on_chooseFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择文件", "", "所有文件 (*.*);;文本文件 (*.txt)");
    if (!fileName.isEmpty()) {
        emit fileSelected(fileName); // 发送信号通知文件选择
    }
}

void ConnectWidget::init()
{
    //初始化波特率的选择
    QPushButton* chooseFileButton = new QPushButton("选择文件", this);
    chooseFileButton->setGeometry(QRect(QPoint(150, 150), QSize(120, 30))); // 根据需要调整位置和大小
    connect(chooseFileButton, &QPushButton::clicked, this, &ConnectWidget::on_chooseFileButton_clicked);

}





void ConnectWidget::setConnectStatus(QString status)
{


}


