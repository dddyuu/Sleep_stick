#include "storageconfigwidget.h"
#include "ui_storageconfigwidget.h"

StorageConfigWidget::StorageConfigWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StorageConfigWidget)
{
    ui->setupUi(this);
    ui->toolBox->setDisabled(true);
}

StorageConfigWidget::~StorageConfigWidget()
{
    delete ui;
}


void StorageConfigWidget::on_pieceRadioButton_clicked()
{
    ui->toolBox->setDisabled(false);

}

void StorageConfigWidget::on_allRadioButton_clicked()
{
    ui->toolBox->setDisabled(true);

}

void StorageConfigWidget::on_chockFAddButton_clicked()
{
    QString filePath = QFileDialog::getExistingDirectory(this,tr("选择文件位置"));
    ui->fileAddEdit->setText(filePath);
}
