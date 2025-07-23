#ifndef FATIGUERESWIDGET_H
#define FATIGUERESWIDGET_H

#include <QWidget>
#include<QVector>
#include<QString>
#include "chart.h"
#include "registerinfo.h"
#include <QFile>
#include <QTextStream>
#include <QResizeEvent>
#include <QShowEvent>  // 添加 showEvent 支持

namespace Ui {
    class FatigueResWidget;
}

class FatigueResWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FatigueResWidget(QWidget* parent = nullptr);
    ~FatigueResWidget();
    //    void initChart();

signals:
    void sendData(quint8 data, uint8_t);
    void updateLabelData(QList<uint8_t> label_data);
    void transSubname(QString);

protected:
    void resizeEvent(QResizeEvent* event) override;  // 添加重写
    void showEvent(QShowEvent* event) override;      // 添加重写

private slots:
    void on_fatigueClass_clicked();

private:
    void positionRegisterInfo();  // 添加辅助函数

    Ui::FatigueResWidget* ui;
    QString SubjectName;
    QMap<QString, QString>Infomation;
    registerInfo* reg;
    Chart* chart_index;
    QList<double> data;
};

#endif // FATIGUERESWIDGET_H
