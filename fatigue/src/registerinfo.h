//#ifndef REGISTERINFO_H
//#define REGISTERINFO_H
//
//#include <QWidget>
//#include<QDebug>
//#include<QDir>
//#include<QJsonDocument>
//#include<QJsonObject>
//#include<QTimer>
//#include<QFileDialog>
//#include "chart.h"
//#include<QTimer>
//namespace Ui {
//class registerInfo;
//}
//
//class registerInfo : public QWidget
//{
//    Q_OBJECT
//public:
//    explicit registerInfo(QWidget *parent = nullptr);
//    ~registerInfo();
//    void init();
//    int countFiles(const QString &directoryPath);//定义mat文件查找函数
//signals:
//    void sendInfo(QMap<QString,QString> Info);
//    void sendSubname(QString subname);//转发文件保存名
//    void sendLabel(QList<int> LocalLabel);//发送数据给chart绘制
//
//private slots:
//    void on_EditInfoBtn_clicked();
//
//    void on_loadUserBtn_clicked();
//
//
//    void on_Samplesload_clicked();
//
//    void on_AccBtn_clicked();
//
//    void on_dataConect_clicked();
//
//    void on_clearRecords_clicked();
//
//    void on_localBtn_clicked();
//
//private:
//    Ui::registerInfo *ui;
//    QMap<QString,QString> Info;
//    void saveInfoJson();
//    void addMessage(const QString &msg);//负责滚动添加信息
//    void displayScroll();//负责滚动信息显示
//    int currentState;
//    QTimer timer;
//    QDate currentDate;
//    QString recordsDirectory;//文件保存路径
//    QList<QString> messages;//保存信息
//    void saveListToFile(const QList<QString>& list, const QString& fileName);
//    bool saveFlag=true;//表示保存(false)或编辑(true)
//    QString SubjectName;
//    QString samplesNums;
//    QString FilePath;
//    int messageCount;
//    QString subeegPath;
//};
//
//#endif // REGISTERINFO_H

#ifndef REGISTERINFO_H
#define REGISTERINFO_H

#include <QWidget>
#include <QMap>

namespace Ui {
    class registerInfo;
}

class registerInfo : public QWidget
{
    Q_OBJECT

public:
    explicit registerInfo(QWidget* parent = nullptr);
    ~registerInfo();
signals:
    void sendSubname(QString subname);
private slots:
    void on_EditInfoBtn_clicked();

private:
    Ui::registerInfo* ui;
    QMap<QString, QString> Info;
    void saveInfoJson();
    QString subeegPath;
};

#endif // REGISTERINFO_H
