#include "BluetoothDeviceController.h"
#include <QDebug>
#include <QBluetoothUuid>
#include <QThread>

BluetoothDeviceController::BluetoothDeviceController(QObject* parent)
    : QObject(parent)
    , m_discoveryAgent(nullptr)
    , m_controller(nullptr)
    , m_service(nullptr)
    , m_connectionState(Disconnected)
    , m_receivingData(false)
    , m_tcpServer(nullptr)
    , m_receivingTcpData(false)
    , m_dataProcessTimer(new QTimer(this))
{
    // 设置特征UUID
    m_rxCharUuid = QBluetoothUuid(QString("6e400003-b5a3-f393-e0a9-e50e24dcca9e"));
    m_txCharUuid = QBluetoothUuid(QString("6e400002-b5a3-f393-e0a9-e50e24dcca9e"));

    initializeBluetooth();

    // 设置数据处理定时器
    m_dataProcessTimer->setSingleShot(true);
    m_dataProcessTimer->setInterval(100); // 100ms延迟处理
}

BluetoothDeviceController::~BluetoothDeviceController()
{
    stopTcpServer();
    disconnectDevice();

    if (m_discoveryAgent) {
        m_discoveryAgent->stop();
        delete m_discoveryAgent;
    }
}

void BluetoothDeviceController::initializeBluetooth()
{
    // 初始化设备发现代理
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        this, &BluetoothDeviceController::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
        this, &BluetoothDeviceController::onScanFinished);
}

void BluetoothDeviceController::startScanning()
{
    if (m_connectionState == Scanning) {
        qWarning() << "已在扫描中";
        return;
    }

    m_discoveredDevices.clear();
    m_connectionState = Scanning;
    emit connectionStateChanged(m_connectionState);

    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    qDebug() << "开始扫描蓝牙设备...";
}

void BluetoothDeviceController::stopScanning()
{
    if (m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
    }
}

void BluetoothDeviceController::connectToDevice(const QBluetoothDeviceInfo& device)
{
    if (m_connectionState == Connected || m_connectionState == Connecting) {
        qWarning() << "设备已连接或正在连接";
        return;
    }

    disconnectDevice(); // 确保之前的连接已断开

    m_connectionState = Connecting;
    emit connectionStateChanged(m_connectionState);

    // 创建低功耗控制器
    m_controller = QLowEnergyController::createCentral(device, this);

    connect(m_controller, &QLowEnergyController::connected,
        this, &BluetoothDeviceController::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
        this, &BluetoothDeviceController::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
        this, &BluetoothDeviceController::onServiceDiscovered);

    // 开始连接
    m_controller->connectToDevice();
    qDebug() << "正在连接设备:" << device.name() << device.address().toString();
}

void BluetoothDeviceController::disconnectDevice()
{
    m_receivingData = false;

    if (m_service) {
        delete m_service;
        m_service = nullptr;
    }

    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
        m_controller = nullptr;
    }

    m_dataBuffer.clear();
    m_connectionState = Disconnected;
    emit connectionStateChanged(m_connectionState);
}

void BluetoothDeviceController::startReceiving()
{
    if (!m_service || m_connectionState != Connected) {
        emit errorOccurred("设备未连接");
        return;
    }

    if (m_receivingData) {
        qWarning() << "已在接收数据";
        return;
    }

    // 查找接收特征
    QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
    if (!rxChar.isValid()) {
        emit errorOccurred("未找到接收特征");
        return;
    }

    // 启用通知
    QLowEnergyDescriptor notification = rxChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    if (notification.isValid()) {
        m_service->writeDescriptor(notification, QByteArray::fromHex("0100"));
        m_receivingData = true;
        qDebug() << "开始接收数据";
    }
    else {
        emit errorOccurred("无法启用通知");
    }
}

void BluetoothDeviceController::stopReceiving()
{
    if (!m_receivingData) return;

    if (m_service) {
        QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
        if (rxChar.isValid()) {
            QLowEnergyDescriptor notification = rxChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            if (notification.isValid()) {
                m_service->writeDescriptor(notification, QByteArray::fromHex("0000"));
            }
        }
    }

    m_receivingData = false;
    m_dataBuffer.clear();
    qDebug() << "停止接收数据";
}

void BluetoothDeviceController::sendCommand(CommandType cmdType)
{
    if (!m_service || m_connectionState != Connected) {
        emit errorOccurred("设备未连接");
        return;
    }

    QLowEnergyCharacteristic txChar = m_service->characteristic(m_txCharUuid);
    if (!txChar.isValid()) {
        emit errorOccurred("未找到发送特征");
        return;
    }

    QByteArray command = getCommandBytes(cmdType);
    if (command.isEmpty()) {
        emit errorOccurred("无效的命令");
        return;
    }

    m_service->writeCharacteristic(txChar, command);
    qDebug() << "发送命令:" << command.toHex(' ').toUpper();
}

void BluetoothDeviceController::startTcpServer(quint16 port)
{
    if (m_tcpServer && m_tcpServer->isListening()) {
        qWarning() << "TCP服务器已启动";
        return;
    }

    if (!m_tcpServer) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection,
            this, &BluetoothDeviceController::onNewTcpConnection);
    }

    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "TCP服务器启动成功，端口:" << port;
    }
    else {
        emit errorOccurred("TCP服务器启动失败: " + m_tcpServer->errorString());
    }
}

void BluetoothDeviceController::stopTcpServer()
{
    if (m_tcpServer) {
        // 断开所有客户端连接
        for (auto client : m_tcpClients) {
            client->disconnectFromHost();
            delete client;
        }
        m_tcpClients.clear();

        m_tcpServer->close();
        delete m_tcpServer;
        m_tcpServer = nullptr;
        qDebug() << "TCP服务器已停止";
    }
}

void BluetoothDeviceController::onDeviceDiscovered(const QBluetoothDeviceInfo& device)
{
    if (!m_discoveredDevices.contains(device)) {
        m_discoveredDevices.append(device);
        emit deviceDiscovered(device);
        qDebug() << "发现设备:" << device.name() << device.address().toString();
    }
}

void BluetoothDeviceController::onScanFinished()
{
    if (m_connectionState == Scanning) {
        m_connectionState = Disconnected;
        emit connectionStateChanged(m_connectionState);
    }
    qDebug() << "扫描完成，发现" << m_discoveredDevices.size() << "个设备";
}

void BluetoothDeviceController::onControllerConnected()
{
    qDebug() << "控制器连接成功，开始发现服务...";
    m_controller->discoverServices();
}

void BluetoothDeviceController::onControllerDisconnected()
{
    qDebug() << "控制器连接断开";
    disconnectDevice();
}

void BluetoothDeviceController::onServiceDiscovered(const QBluetoothUuid& serviceUuid)
{
    qDebug() << "发现服务:" << serviceUuid.toString();

    // 创建服务对象（这里假设使用标准的Nordic UART服务UUID）
    QBluetoothUuid uartServiceUuid(QString("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));
    if (serviceUuid == uartServiceUuid) {
        m_service = m_controller->createServiceObject(serviceUuid, this);
        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged,
                this, &BluetoothDeviceController::onServiceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged,
                this, &BluetoothDeviceController::onCharacteristicChanged);

            m_service->discoverDetails();
        }
    }
}

void BluetoothDeviceController::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state == QLowEnergyService::ServiceDiscovered) {
        qDebug() << "服务详细信息发现完成";
        m_connectionState = Connected;
        emit connectionStateChanged(m_connectionState);

        // 自动开始接收数据
        startReceiving();
    }
}

void BluetoothDeviceController::onCharacteristicChanged(const QLowEnergyCharacteristic& characteristic,
    const QByteArray& newValue)
{
    if (characteristic.uuid() == m_rxCharUuid && m_receivingData) {
        processReceivedData(newValue);
    }
}

void BluetoothDeviceController::onNewTcpConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* client = m_tcpServer->nextPendingConnection();
        m_tcpClients.append(client);

        connect(client, &QTcpSocket::readyRead,
            this, &BluetoothDeviceController::onTcpDataReceived);
        connect(client, &QTcpSocket::disconnected,
            this, &BluetoothDeviceController::onTcpDisconnected);

        qDebug() << "新TCP客户端连接:" << client->peerAddress().toString();
    }
}

void BluetoothDeviceController::onTcpDataReceived()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    processReceivedData(data);
}

void BluetoothDeviceController::onTcpDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (client) {
        m_tcpClients.removeOne(client);
        client->deleteLater();
        qDebug() << "TCP客户端断开连接";
    }
}

void BluetoothDeviceController::processReceivedData(const QByteArray& data)
{
    QMutexLocker locker(&m_bufferMutex);

    // 将新数据添加到缓冲区
    m_dataBuffer.append(data);

    // 处理缓冲区中的完整数据包
    while (m_dataBuffer.length() >= EXPECTED_LENGTH) {
        QByteArray packet = m_dataBuffer.left(EXPECTED_LENGTH);
        m_dataBuffer.remove(0, EXPECTED_LENGTH);

        // 解析数据
        DataParser::ParsedData parsedData = DataParser::parseData(packet);
        if (parsedData.valid) {
            emit dataReceived(parsedData);
        }
        else {
            qWarning() << "数据解析失败";
        }
    }
}

QByteArray BluetoothDeviceController::getCommandBytes(CommandType cmdType)
{
    QByteArray command;

    switch (cmdType) {
    case StartCommand:
        command = QByteArray::fromHex("010203"); // 示例开始命令
        break;
    case StopCommand:
        command = QByteArray::fromHex("040506"); // 示例停止命令
        break;
    case StatusCommand:
        command = QByteArray::fromHex("070809"); // 示例状态查询命令
        break;
    default:
        break;
    }

    return command;
}