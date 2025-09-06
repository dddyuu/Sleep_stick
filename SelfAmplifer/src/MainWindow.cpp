// src/MainWindow/MainWindow.cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QListWidgetItem>
#include <QMessageBox>
#include <QVariant>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , controller(new BluetoothDeviceController(this))
{
    ui->setupUi(this);

    // 连接按钮信号槽
    connect(ui->btnScan, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->btnStartRecv, &QPushButton::clicked, this, &MainWindow::onStartRecvClicked);
    connect(ui->btnStopRecv, &QPushButton::clicked, this, &MainWindow::onStopRecvClicked);
    connect(ui->btnSendTestCmd, &QPushButton::clicked, this, &MainWindow::onSendTestCmdClicked);
    connect(ui->btnStartTcpServer, &QPushButton::clicked, this, &MainWindow::onStartTcpServerClicked);
    connect(ui->btnStopTcpServer, &QPushButton::clicked, this, &MainWindow::onStopTcpServerClicked);

    // 连接蓝牙控制器信号槽
    connect(controller, &BluetoothDeviceController::deviceDiscovered, this, &MainWindow::onDeviceDiscovered);
    connect(controller, &BluetoothDeviceController::connectionStateChanged, this, &MainWindow::onConnectionStateChanged);
    connect(controller, &BluetoothDeviceController::dataReceived, this, &MainWindow::onDataReceived);
    connect(controller, &BluetoothDeviceController::errorOccurred, this, &MainWindow::onErrorOccurred);

    updateUIState();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onScanClicked()
{
    ui->textLog->append("开始扫描设备...");
    ui->listDevices->clear();
    controller->startScanning();
}

void MainWindow::onConnectClicked()
{
    QListWidgetItem* item = ui->listDevices->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请选择一个设备连接");
        return;
    }
    QVariant v = item->data(Qt::UserRole);
    QBluetoothDeviceInfo device = v.value<QBluetoothDeviceInfo>();

    controller->connectToDevice(device);
    ui->textLog->append("尝试连接设备: " + device.name() + " (" + device.address().toString() + ")");
}

void MainWindow::onDisconnectClicked()
{
    controller->disconnectDevice();
    ui->textLog->append("断开连接");
}

void MainWindow::onStartRecvClicked()
{
    // 在开始接收时先发送启动命令
    controller->sendCommand(BluetoothDeviceController::StartCommand);
    controller->startReceiving();
    ui->textLog->append("发送启动命令并开始接收数据");
}

void MainWindow::onStopRecvClicked()
{
    // 在停止接收时发送停止命令
    controller->sendCommand(BluetoothDeviceController::StopCommand);
    controller->stopReceiving();
    ui->textLog->append("发送停止命令并停止接收数据");
}

void MainWindow::onSendTestCmdClicked()
{
    // 发送启动命令而不是测试命令
    controller->sendCommand(BluetoothDeviceController::StartCommand);
    ui->textLog->append("发送启动命令");
}

void MainWindow::onStartTcpServerClicked()
{
    controller->startTcpServer(8888);
    ui->textLog->append("TCP服务器启动，端口: 8888");
}

void MainWindow::onStopTcpServerClicked()
{
    controller->stopTcpServer();
    ui->textLog->append("TCP服务器停止");
}

void MainWindow::onDeviceDiscovered(const QBluetoothDeviceInfo& device)
{
    QString desc = device.name() + " (" + device.address().toString() + ")";
    QListWidgetItem* item = new QListWidgetItem(desc, ui->listDevices);
    item->setData(Qt::UserRole, QVariant::fromValue(device));
    ui->listDevices->addItem(item);
    ui->textLog->append("发现设备：" + desc);
}

void MainWindow::onConnectionStateChanged(BluetoothDeviceController::ConnectionState state)
{
    QString text;
    switch (state) {
    case BluetoothDeviceController::Disconnected: text = "未连接"; break;
    case BluetoothDeviceController::Scanning: text = "扫描中"; break;
    case BluetoothDeviceController::Connecting: text = "连接中"; break;
    case BluetoothDeviceController::Connected: text = "已连接"; break;
    }
    ui->textLog->append("连接状态: " + text);
    updateUIState();
}

void MainWindow::onDataReceived(const DataParser::ParsedData& data)
{
    // 显示更详细的数据信息
    QString log = QString("接收到数据 - 序号: %1, 电池百分比: %2, EEG通道数: %3, 音频数据长度: %4")
        .arg(data.order_num)
        .arg(data.battery)
        .arg(data.eeg_data.size())  // 现在有size()方法了
        .arg(data.video_data.size());
    ui->textLog->append(log);

    // 显示原始数据的前几个字节
    QString rawData = QString("原始数据前20字节: %1")
        .arg(QString(data.rawdata.left(20).toHex(' ').toUpper()));
    ui->textLog->append(rawData);
}

void MainWindow::onErrorOccurred(const QString& error)
{
    ui->textLog->append("[错误] " + error);
}

void MainWindow::updateUIState()
{
    auto state = controller->connectionState();
    bool connected = (state == BluetoothDeviceController::Connected);

    ui->btnConnect->setEnabled(state == BluetoothDeviceController::Disconnected);
    ui->btnDisconnect->setEnabled(connected);
    ui->btnStartRecv->setEnabled(connected);
    ui->btnStopRecv->setEnabled(connected);
    ui->btnSendTestCmd->setEnabled(connected);
    ui->btnScan->setEnabled(state != BluetoothDeviceController::Scanning);
}
