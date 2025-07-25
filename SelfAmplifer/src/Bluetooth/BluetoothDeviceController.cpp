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

    // 【修改1】添加更完整的错误处理
    connect(m_controller, QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
        [this](QLowEnergyController::Error error) {
            qWarning() << "QLowEnergyController error:" << error;
            emit errorOccurred("蓝牙控制器错误: " + QString::number(static_cast<int>(error)));
        });

    connect(m_controller, &QLowEnergyController::connected,
        this, &BluetoothDeviceController::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
        this, &BluetoothDeviceController::onControllerDisconnected);
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
        this, &BluetoothDeviceController::onServiceDiscovered);

    // 【修改2】添加服务发现完成的处理
    connect(m_controller, &QLowEnergyController::discoveryFinished,
        this, &BluetoothDeviceController::onServiceDiscoveryFinished);

    // 开始连接
    m_controller->connectToDevice();
    qDebug() << "正在连接设备:" << device.name() << device.address().toString();
}

void BluetoothDeviceController::disconnectDevice()
{
    m_receivingData = false;

    // 【修改3】先停止通知再删除服务
    if (m_service) {
        if (m_service->state() == QLowEnergyService::ServiceDiscovered) {
            QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
            if (rxChar.isValid()) {
                QLowEnergyDescriptor notification = rxChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                if (notification.isValid()) {
                    m_service->writeDescriptor(notification, QByteArray::fromHex("0000"));
                }
            }
        }
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_controller) {
        if (m_controller->state() != QLowEnergyController::UnconnectedState) {
            m_controller->disconnectFromDevice();
        }
        m_controller->deleteLater();
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

    // 确保服务状态正确
    if (m_service->state() != QLowEnergyService::ServiceDiscovered) {
        emit errorOccurred("服务未就绪");
        return;
    }

    // 查找接收特征
    QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
    if (!rxChar.isValid()) {
        emit errorOccurred("未找到接收特征");
        return;
    }

    qDebug() << "RX特征详细信息:";
    qDebug() << "  UUID:" << rxChar.uuid().toString();
    qDebug() << "  属性:" << QString::number(rxChar.properties(), 16);
    qDebug() << "  描述符数量:" << rxChar.descriptors().size();

    // 检查特征属性 - 支持Notify或Indicate
    if (!(rxChar.properties() & (QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Indicate))) {
        emit errorOccurred("接收特征不支持通知或指示");
        return;
    }

    //添加描述符写入完成的处理
    connect(m_service, &QLowEnergyService::descriptorWritten,
        this, &BluetoothDeviceController::onDescriptorWritten, Qt::UniqueConnection);

    // 启用通知
    QLowEnergyDescriptor notification = rxChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    if (notification.isValid()) {
        qDebug() << "找到CCCD描述符，正在启用通知...";
        qDebug() << "CCCD UUID:" << notification.uuid().toString();

        // 根据特征属性选择通知或指示
        QByteArray value;
        if (rxChar.properties() & QLowEnergyCharacteristic::Notify) {
            value = QByteArray::fromHex("0100"); // 启用通知
            qDebug() << "启用Notify";
        }
        else if (rxChar.properties() & QLowEnergyCharacteristic::Indicate) {
            value = QByteArray::fromHex("0200"); // 启用指示
            qDebug() << "启用Indicate";
        }

        m_service->writeDescriptor(notification, value);
        // 注意：这里不立即设置m_receivingData，等待写入完成后再设置
    }
    else {
        emit errorOccurred("无法找到CCCD描述符");

        // 【新增】列出所有可用的描述符进行调试
        qDebug() << "可用描述符:";
        for (const auto& desc : rxChar.descriptors()) {
            qDebug() << "  描述符UUID:" << desc.uuid().toString();
            qDebug() << "  描述符名称:" << desc.name();
        }
    }
}

void BluetoothDeviceController::stopReceiving()
{
    if (!m_receivingData) return;

    if (m_service && m_service->state() == QLowEnergyService::ServiceDiscovered) {
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

    // 确保服务状态正确
    if (m_service->state() != QLowEnergyService::ServiceDiscovered) {
        emit errorOccurred("服务未就绪");
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
            client->deleteLater();
        }
        m_tcpClients.clear();

        m_tcpServer->close();
        m_tcpServer->deleteLater();
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

    // 只处理目标服务，避免创建过多服务对象
    QBluetoothUuid uartServiceUuid(QString("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));
    if (serviceUuid == uartServiceUuid) {
        qDebug() << "找到目标UART服务";
        m_service = m_controller->createServiceObject(serviceUuid, this);
        if (!m_service) {
            qWarning() << "无法创建服务对象:" << serviceUuid.toString();
            emit errorOccurred("无法创建服务对象");
            return;
        }

        connect(m_service, &QLowEnergyService::stateChanged,
            this, &BluetoothDeviceController::onServiceStateChanged);

        //数据连接
        connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &BluetoothDeviceController::onCharacteristicChanged);

        //添加更多服务信号连接
        connect(m_service, &QLowEnergyService::characteristicRead,
            this, &BluetoothDeviceController::onCharacteristicRead);
        connect(m_service, &QLowEnergyService::characteristicWritten,
            this, &BluetoothDeviceController::onCharacteristicWritten);
        connect(m_service, &QLowEnergyService::descriptorRead,
            this, &BluetoothDeviceController::onDescriptorRead);
        connect(m_service, &QLowEnergyService::descriptorWritten,
            this, &BluetoothDeviceController::onDescriptorWritten);

        // 添加错误处理
        connect(m_service, QOverload<QLowEnergyService::ServiceError>::of(&QLowEnergyService::error),
            [this](QLowEnergyService::ServiceError error) {
                qWarning() << "服务错误:" << error;
                emit errorOccurred("服务错误: " + QString::number(static_cast<int>(error)));
            });

        // 发现服务详细信息
        m_service->discoverDetails();
    }
}

// 【修改9】添加服务发现完成处理函数
void BluetoothDeviceController::onServiceDiscoveryFinished()
{
    qDebug() << "服务发现完成";
    if (!m_service) {
        emit errorOccurred("未找到目标服务");
        disconnectDevice();
    }
}

void BluetoothDeviceController::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    qDebug() << "服务状态变化:" << state;

    if (state == QLowEnergyService::ServiceDiscovered) {
        qDebug() << "服务详细信息发现完成";

        // 【修改10】验证特征是否存在
        QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
        QLowEnergyCharacteristic txChar = m_service->characteristic(m_txCharUuid);

        if (!rxChar.isValid()) {
            emit errorOccurred("未找到RX特征");
            return;
        }

        if (!txChar.isValid()) {
            emit errorOccurred("未找到TX特征");
            return;
        }

        qDebug() << "RX特征属性:" << rxChar.properties();
        qDebug() << "TX特征属性:" << txChar.properties();

        m_connectionState = Connected;
        emit connectionStateChanged(m_connectionState);

        // 【修改11】延迟启动接收，确保连接稳定
        QTimer::singleShot(1000, this, [this]() {
            qDebug() << "自动开始接收数据";
            startReceiving();
            });
    }
    else if (state == QLowEnergyService::InvalidService) {
        emit errorOccurred("服务无效");
        disconnectDevice();
    }
}

// 【新增】添加所有服务操作的回调函数
void BluetoothDeviceController::onCharacteristicRead(const QLowEnergyCharacteristic& characteristic, const QByteArray& value)
{
    qDebug() << "特征读取完成 - UUID:" << characteristic.uuid().toString() << "值:" << value.toHex(' ');
}

void BluetoothDeviceController::onCharacteristicWritten(const QLowEnergyCharacteristic& characteristic, const QByteArray& newValue)
{
    qDebug() << "特征写入完成 - UUID:" << characteristic.uuid().toString() << "值:" << newValue.toHex(' ');
}

void BluetoothDeviceController::onDescriptorRead(const QLowEnergyDescriptor& descriptor, const QByteArray& value)
{
    qDebug() << "描述符读取完成 - UUID:" << descriptor.uuid().toString() << "值:" << value.toHex(' ');
}

void BluetoothDeviceController::onDescriptorWritten(const QLowEnergyDescriptor& descriptor, const QByteArray& newValue)
{
    qDebug() << "描述符写入完成 - UUID:" << descriptor.uuid().toString() << "值:" << newValue.toHex(' ');

    // 检查是否是CCCD描述符的写入
    if (descriptor.uuid() == QBluetoothUuid::ClientCharacteristicConfiguration) {
        QByteArray expectedNotify = QByteArray::fromHex("0100");
        QByteArray expectedIndicate = QByteArray::fromHex("0200");

        if (newValue == expectedNotify || newValue == expectedIndicate) {
            qDebug() << "通知订阅成功！";
            m_receivingData = true;

            // 【新增】尝试主动读取一次特征值来测试连接
            QLowEnergyCharacteristic rxChar = m_service->characteristic(m_rxCharUuid);
            if (rxChar.isValid() && (rxChar.properties() & QLowEnergyCharacteristic::Read)) {
                qDebug() << "尝试主动读取RX特征...";
                m_service->readCharacteristic(rxChar);
            }

            // 【修改14】立即发送启动命令来激活设备
            QTimer::singleShot(1000, this, [this]() {
                qDebug() << "发送启动命令来激活设备...";
                sendCommand(StartCommand);
                });
        }
        else {
            qWarning() << "通知订阅可能失败，写入值:" << newValue.toHex(' ');
        }
    }
}

void BluetoothDeviceController::onCharacteristicChanged(const QLowEnergyCharacteristic& characteristic,
    const QByteArray& newValue)
{
    // 详细信息
    // qDebug() << "特征变化 - UUID:" << characteristic.uuid().toString();
    // qDebug() << "期望的RX UUID:" << m_rxCharUuid.toString();
    // qDebug() << "数据长度:" << newValue.length();
    // qDebug() << "接收状态:" << m_receivingData;

    if (characteristic.uuid() == m_rxCharUuid && m_receivingData) {
        // qDebug() << "处理接收到的数据:" << newValue.toHex(' ').toUpper();
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

    // 寻找帧头标识
    const QByteArray FRAME_HEADER = QByteArray::fromHex("41495290"); // 帧头

    // qDebug() << "当前缓冲区大小:" << m_dataBuffer.size() << "数据:" << m_dataBuffer.left(20).toHex(' ').toUpper();

    // 处理缓冲区中的完整数据包
    while (m_dataBuffer.length() >= EXPECTED_LENGTH) {
        // 查找帧头
        int headerIndex = m_dataBuffer.indexOf(FRAME_HEADER);
        if (headerIndex == -1) {
            // 没有找到帧头，保留最后几个字节以防帧头被分割
            if (m_dataBuffer.length() > FRAME_HEADER.length()) {
                m_dataBuffer.remove(0, m_dataBuffer.length() - FRAME_HEADER.length() + 1);
            }
            break;
        }

        // 如果帧头不在开始位置，移除之前的无效数据
        if (headerIndex > 0) {
            m_dataBuffer.remove(0, headerIndex);
        }

        // 检查是否有完整的数据包
        if (m_dataBuffer.length() < EXPECTED_LENGTH) {
            break;
        }

        QByteArray packet = m_dataBuffer.left(EXPECTED_LENGTH);
        m_dataBuffer.remove(0, EXPECTED_LENGTH);

        // qDebug() << "处理完整数据包，长度:" << packet.length();
        // qDebug() << "数据包前20字节:" << packet.left(20).toHex(' ').toUpper();

        // 解析数据
        //DataParser::ParsedData parsedData = DataParser::parseData(packet);
        DataParser::ParsedData parsedData = DataParser::parseDataStatic(packet);
        if (parsedData.valid) {
            // qDebug() << "数据解析成功！序号:" << parsedData.order_num;
            emit dataReceived(parsedData);
        }
        else {
            qDebug() << "数据解析失败，丢弃此包";
        }
    }
}

QByteArray BluetoothDeviceController::getCommandBytes(CommandType cmdType)
{
    QByteArray command;

    // 命令格式
    switch (cmdType) {
    case StartCommand:
        // Python: bytes([0x48, 0x4E, 0x55, 0x9F, 0x03, 0x00, 0x00, 0x00])
        command = QByteArray::fromHex("484E559F03000000");
        break;
    case StopCommand:
        // Python: bytes([0x48, 0x4E, 0x55, 0x9F, 0x04, 0x00, 0x00, 0x00])
        command = QByteArray::fromHex("484E559F04000000");
        break;
    case StatusCommand:
        // Python: bytes([0x48, 0x4E, 0x55, 0x9F, 0x05, 0x08, 0x00, 0x00])
        command = QByteArray::fromHex("484E559F05080000");
        break;
    default:
        break;
    }

    return command;
}
