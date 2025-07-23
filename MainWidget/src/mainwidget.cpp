#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "QPushButton"
#include "QFile"
#include "QDebug"
#include<QFileDialog>
#include<QProcess>
MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    init();
}

MainWidget::~MainWidget()
{
    delete ui;
}
void MainWidget::init()
{

    button_group=new QButtonGroup;
    mediawidget=new MediaWidget(this);
    mishow=new MIShow();
    mediawidget->setWindowFlag(Qt::Dialog);

    //QStringList task_name={"测试demo","采集数据"};
    QStringList task_name = {"采集数据" };
    ui->btn_layout->setRowStretch(2,1);
    ui->btn_layout->setColumnStretch(4,1);
    for(int i=0;i<1;i++)
    {
        QPushButton *button=new QPushButton;
        button->setText(task_name[i]);
        button->setStyleSheet("QPushButton { background-color: red;font: 25pt ;border-radius:15px; }");
        button->setMinimumSize(400,200);
        ui->btn_layout->addWidget(button,int(i/3),i%3);
        button_group->addButton(button,i);
    }
    connect(button_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
          [=](int id){
          //if(id==0)
          //{
          //    readLabels("seed-stimulation");
          //    mediawidget->setVideoPath("seed-stimulation");
          //    mediawidget->show();
          //}
          if(id==0)
          {
              QString fileName = QFileDialog::getOpenFileName(this, tr("选择exe文件"), "", tr("exe Files (*.exe)"));
              QProcess::startDetached(fileName);
          }
      
    });

    connect(mediawidget,&MediaWidget::currentIndexChanged,this,&MainWidget::currentIndexChanged);
    connect(mediawidget,&MediaWidget::start,this,&MainWidget::start);
    connect(mediawidget,&MediaWidget::stop,this,&MainWidget::stop);
    connect(mediawidget,&MediaWidget::resetSignal,this,&MainWidget::resetSignal);
    connect(mediawidget,&MediaWidget::filenameChenckIn,this,&MainWidget::filenameChenckIn);
    connect(mediawidget,&MediaWidget::subjectRegistered,this,&MainWidget::subjectRegistered);
    connect(mediawidget,&MediaWidget::videoEnd,this,&MainWidget::videoEnd);

//    connect(mishow,&MIShow::startSignal,this,&MainWidget::start);
    connect(mishow,&MIShow::resetSignal,this,&MainWidget::resetSignal);
    connect(mishow,&MIShow::markChange,this,&MainWidget::markChange);
     connect(mishow,&MIShow::filenameChenckIn,this,&MainWidget::filenameChenckIn);

}

void MainWidget::readLabels(QString dir)
{
      QString path=dir+"/seed-stimulation.csv";
      QFile file(path);
      bool isok=file.open(QIODevice::ReadOnly);
      if(isok)
      {
          QVector<quint8> labels;
          QString file_data=file.readAll();
          QStringList _file_data=file_data.split("\n");
          _file_data.removeAt(0);
          for(auto row_data:_file_data)
          {
              QStringList _row_data=row_data.split(",");
              if(_row_data.size()>1)
              {
                  QString label=_row_data[1];
                  quint8 value=label.toUInt();
                  labels.append(value);
              }
          }
          emit labelsChanged(labels);
      }
}
