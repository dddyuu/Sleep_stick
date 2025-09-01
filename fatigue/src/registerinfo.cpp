
#include "registerinfo.h"
#include "ui_registerinfo.h"
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

registerInfo::registerInfo(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::registerInfo)
{
    ui->setupUi(this);
    subeegPath = "D://SubEEG//";
    // 设置窗口的背景颜色为白色
    this->setStyleSheet("#{background-color: rgba(240,240,240,80%);}");
    // 设置所有编辑框为不可编辑状态
    ui->NameEdit->setEnabled(false);
    ui->SexEdit->setEnabled(false);
    ui->AgeEdit->setEnabled(false);
    ui->HeightEdit->setEnabled(false);
    ui->WeightEdit->setEnabled(false);

    // 设置初始值（可选）
    //ui->SexEdit->setText("男");
    //ui->AgeEdit->setText("30");
    //ui->HeightEdit->setText("175");
    //ui->WeightEdit->setText("70");

    // 保存初始信息
    Info["name:"] = ui->NameEdit->text();
    Info["Age:"] = ui->AgeEdit->text();
    Info["Height:"] = ui->HeightEdit->text();
    Info["Weight:"] = ui->WeightEdit->text();
    Info["Sex:"] = ui->SexEdit->text();
}

registerInfo::~registerInfo()
{
    delete ui;
}

void registerInfo::on_EditInfoBtn_clicked()
{
    static bool saveFlag = true; // true表示编辑状态，false表示保存状态

    if (saveFlag) {
        // 切换到编辑模式
        ui->EditInfoBtn->setText("保存");
        ui->NameEdit->setEnabled(true);
        ui->SexEdit->setEnabled(true);
        ui->AgeEdit->setEnabled(true);
        ui->HeightEdit->setEnabled(true);
        ui->WeightEdit->setEnabled(true);
    }
    else {
        // 保存信息
        ui->EditInfoBtn->setText("编辑");
        ui->NameEdit->setEnabled(false);
        ui->SexEdit->setEnabled(false);
        ui->AgeEdit->setEnabled(false);
        ui->HeightEdit->setEnabled(false);
        ui->WeightEdit->setEnabled(false);

        // 更新信息
        Info["name:"] = ui->NameEdit->text();
        Info["Age:"] = ui->AgeEdit->text();
        Info["Height:"] = ui->HeightEdit->text();
        Info["Weight:"] = ui->WeightEdit->text();
        Info["Sex:"] = ui->SexEdit->text();
        QString Subname = subeegPath+ Info["name:"] +".mat";
        QDir dir = QFileInfo(Subname).absoluteDir();//获取文件的绝对路径
        if(!dir.exists())//目录不存在
        {
            bool created = dir.mkpath(".");
        }
        // 保存到JSON文件
        saveInfoJson();
        qDebug()<<Subname;
        emit sendSubname(Subname);
    }

    saveFlag = !saveFlag;
}

void registerInfo::saveInfoJson()
{
    // 创建 "Subjects" 文件夹
    QString folderPath = "Subjects";
    QDir dir;
    if (!dir.exists(folderPath)) {
        dir.mkpath(folderPath);
    }

    // 创建JSON对象
    QJsonObject jsonObject;
    for (auto it = Info.begin(); it != Info.end(); ++it) {
        jsonObject[it.key()] = it.value();
    }

    // 创建JSON文档
    QJsonDocument jsonDoc(jsonObject);

    // 保存到文件
    QString filePath = folderPath + "/user_info.json";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonDoc.toJson());
        file.close();
        qDebug() << "个人信息已保存到:" << filePath;
    }
    else {
        qDebug() << "保存失败:" << file.errorString();
    }
}
