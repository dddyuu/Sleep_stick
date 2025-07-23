#include "indexwidget.h"
#include "ui_indexwidget.h"
#include "QIcon"
#include "QDebug"
IndexWidget::IndexWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::IndexWidget)
{
    ui->setupUi(this);
    init();
}

IndexWidget::~IndexWidget()
{
    delete ui;
}
void IndexWidget::init()
{
    lastIndex=0;
    //初始化指针
    this->tabBar=new QTabBar();
    this->widget=new QStackedWidget();
    //设置布局
    ui->tabs->addWidget(tabBar);
    ui->verticalLayout->addWidget(widget);

    //初始化选项卡
//    this->tabBar->addTab(u8"上一页");
//修改

    //原有样式表
    /*"QTabWidget::pane{"
                                    "   border:none;}"
                                    "QTabWidget::tab-bar {"
                                    "   left: 5px;"
                                    "}"
                                    "QTabBar::tab {"
                                    "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0.278409 #000000, stop:1 #1E90FF);"
                                    "   border: 2px solid #C4C4C3;"
                                    "   border-bottom-color: #C2C7CB;"
                                    "   border-top-left-radius: 10px;"
                                    "   border-top-right-radius: 10px;"
                                    "   min-width: 80px;"
                                    "   padding: 5px;"
                                    "   color:white;"
                                    "   margin:10px 2px 0px 2px;"
                                    "}"*/

//    "QTabBar::tab:selected{"
//    "   background-color: #00BFFF;"
//    "}"


    this->tabBar->setStyleSheet(
                "QTabBar{qproperty-drawBase:0;}"
                "QTabWidget::pane{"

                "}"
                "QTabBar::tab {"
                "font:24px 黑体;"
                "border:none;"
                "   min-height:50px;"
                "   min-width: 90px;"
                "   padding: 5px;"
                "   color:white;"
                "   margin:10px 2px 0px 2px;"
                "}"




               "QTabBar::tab:selected{"
                "   background-color: #1E90FF;"
                "   background-image: url(:/image/tz.png);"
                "   color:white;"
                "   border-radius:5px;"
                "}"

                "QTabBar::tab:pressed{"
                "   background-color: #1E90FF;"
                "   background-image: url(:/image/tz.png);"
                "   color:white;"
                "}"


                );


    //设置最大化最小化图标   因为在代码中实现图片会导致按压成exe文件后会看不见，所以此步骤转到qss中完成操作。
//    ui->min->setIcon(QIcon("./image/min.png"));
//    ui->max->setIcon(QIcon("./image/max.png"));
//    ui->close->setIcon(QIcon("./image/close.png"));
 //   setStyle();
//    //信号链接
    connect(this->tabBar,&QTabBar::tabBarClicked,this,&IndexWidget::tabClickSlot);
}

void IndexWidget::setStyle()
{
}

void IndexWidget::minMax()
{

}

void IndexWidget::addWidget(QString name, QWidget *widget)
{
    this->tabBar->addTab(name);
    this->widget->addWidget(widget);
}

void IndexWidget::tabClickSlot(int index)
{
    if(this->widget->count()==0)
    {
        return;
    }
//    if(index==0)
//    {
//        this->widget->setCurrentIndex(lastIndex);
//        this->tabBar->setCurrentIndex(lastIndex+1);
//    }
//    else
//    {
//        this->lastIndex=this->widget->currentIndex();
//        this->widget->setCurrentIndex(index-1);
//    }
    this->widget->setCurrentIndex(index);
}



void IndexWidget::on_min_clicked()
{
    emit minSingal();
}

void IndexWidget::on_max_clicked()
{
    emit maxSingal();
}

void IndexWidget::on_close_clicked()
{
//    QCoreApplication::quit();
    emit closeSingal();
}


