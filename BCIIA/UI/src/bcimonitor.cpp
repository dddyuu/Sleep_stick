#include "bcimonitor.h"
#include "bcimonitor.h"
#include "ui_bcimonitor.h"
BCIMonitor::BCIMonitor(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::BCIMonitor)
{
    ui->setupUi(this);
    init();
    setConnect();
}
BCIMonitor::~BCIMonitor()
{
    delete ui;
}
void BCIMonitor::setCurveLabels(QStringList labels)
{
    curvegroup->setCurveLabels(labels);
    curvegroup->setCurveNum(labels.size());
}
void BCIMonitor::monitorEnable(bool status)
{
    if (status)
    {
        toolbar->enableAction();
    }
    else
    {
        toolbar->disabledAction();
    }
    
}
void BCIMonitor::append(QList<double> value)
{
    curvegroup->append(value);
}
void BCIMonitor::init()
{
    initTool();
    initStatus();
    initCurveGroup();
}
void BCIMonitor::setConnect()
{
    setToolConnect();
}
void BCIMonitor::initTool()
{
    toolbar = new MonitorToolBar();
    this->addToolBar(toolbar);
}
void BCIMonitor::setToolConnect()
{
    connect(toolbar, &MonitorToolBar::nextSignal, curvegroup, &CurveGroup::next);
    connect(toolbar, &MonitorToolBar::lastSignal, curvegroup, &CurveGroup::last);
    //通道数增减
    connect(toolbar, &MonitorToolBar::increaseChannelSignal, curvegroup, &CurveGroup::increaseGroupChannel);
    connect(toolbar, &MonitorToolBar::ReduceChannelSignal, curvegroup, &CurveGroup::reduceGroupChannel);
    //y轴的增减
    connect(toolbar, &MonitorToolBar::increasescaleSignal, curvegroup, &CurveGroup::increaseScale);
    connect(toolbar, &MonitorToolBar::ReducescaleSignal, curvegroup, &CurveGroup::reduceScale);
    //x轴的增减
    connect(toolbar, &MonitorToolBar::increasedotSignal, curvegroup, &CurveGroup::increaseXRange);
    connect(toolbar, &MonitorToolBar::ReducedotSignal, curvegroup, &CurveGroup::reduceScale);

    //采集器监听
    connect(toolbar, &MonitorToolBar::monitorSignal, this, &BCIMonitor::startMonitor);
    connect(toolbar, &MonitorToolBar::stopmonitorSignal, this, &BCIMonitor::stopMonitor);
    //采集器插件
    connect(toolbar, &MonitorToolBar::pluginSignal, this, &BCIMonitor::openPulgin);
    //采集器连接
    connect(toolbar, &MonitorToolBar::connectSignal, this, &BCIMonitor::openConnect);
    //文件操作
    connect(toolbar, &MonitorToolBar::startSignal, this, &BCIMonitor::startSave);
    connect(toolbar, &MonitorToolBar::pauseSignal, this, &BCIMonitor::pauseSave);
    connect(toolbar, &MonitorToolBar::stopSignal, this, &BCIMonitor::stopSave);

    //解密
    connect(toolbar, &MonitorToolBar::decodeSignal, this, &BCIMonitor::decodeSignal);
    connect(toolbar, &MonitorToolBar::encodeSignal, this, &BCIMonitor::encodeSignal);
    

}
void BCIMonitor::initStatus()
{
    this->statusbar = new QStatusBar();
    this->setStatusBar(statusbar);
}
void BCIMonitor::initCurveGroup()
{
    curvegroup = new CurveGroup();
    this->setCentralWidget(curvegroup);
}
