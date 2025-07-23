#ifndef BLUETOOTHDEVICECONTROLLER_H
#define BLUETOOTHDEVICECONTROLLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QBluetoothDeviceInfo>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include "DataParser.h"

/**
 * @brief 蓝牙设备控制器类 - 管理蓝牙连接和数据传输
 */
class BluetoothDeviceController : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothDeviceController(QObject* parent = nullptr);
    ~BluetoothDeviceController();

    // 连接状态枚举
    enum ConnectionState {
        Disconnected,
        Scanning,
        Connecting,
        Connected
    };

    // 命令类型枚举
    enum CommandType {
        StartCommand,
        StopCommand,
        StatusCommand
    };

    ConnectionState connectionState() const { return m_connectionState; }  // 新增getter

public slots:
    /**
     * @brief 开始扫描蓝牙设备
     */
    void startScanning();

    /**
     * @brief 停止扫描
     */
    void stopScanning();

    /**
     * @brief 连接到指定设备
     * @param device 设备信息
     */
    void connectToDevice(const QBluetoothDeviceInfo& device);

    /**
     * @brief 断开连接
     */
    void disconnectDevice();

    /**
     * @brief 开始接收数据
     */
    void startReceiving();

    /**
     * @brief 停止接收数据
     */
    void stopReceiving();

    /**
     * @brief 发送命令到设备
     * @param cmdType 命令类型
     */
    void sendCommand(CommandType cmdType);

    /**
     * @brief 启动TCP服务器
     * @param port 端口号
     */
    void startTcpServer(quint16 port = 8888);

    /**
     * @brief 停止TCP服务器
     */
    void stopTcpServer();

private slots:
    /**
     * @brief 处理设备发现
     * @param device 发现的设备
     */
    void onDeviceDiscovered(const QBluetoothDeviceInfo& device);

    /**
     * @brief 处理扫描完成
     */
    void onScanFinished();

    /**
     * @brief 处理控制器连接
     */
    void onControllerConnected();

    /**
     * @brief 处理控制器断开
     */
    void onControllerDisconnected();

    /**
     * @brief 处理服务发现完成
     */
    void onServiceDiscovered(const QBluetoothUuid& serviceUuid);

    /**
     * @brief 处理服务状态改变
     * @param state 新状态
     */
    void onServiceStateChanged(QLowEnergyService::ServiceState state);

    /**
     * @brief 处理特征值改变
     * @param characteristic 特征
     * @param newValue 新值
     */
    void onCharacteristicChanged(const QLowEnergyCharacteristic& characteristic,
        const QByteArray& newValue);

    /**
     * @brief 处理TCP新连接
     */
    void onNewTcpConnection();

    /**
     * @brief 处理TCP数据接收
     */
    void onTcpDataReceived();

    /**
     * @brief 处理TCP连接断开
     */
    void onTcpDisconnected();

signals:
    /**
     * @brief 设备发现信号
     * @param device 设备信息
     */
    void deviceDiscovered(const QBluetoothDeviceInfo& device);

    /**
     * @brief 连接状态改变信号
     * @param state 新状态
     */
    void connectionStateChanged(ConnectionState state);

    /**
     * @brief 数据接收信号
     * @param data 解析后的数据
     */
    void dataReceived(const DataParser::ParsedData& data);

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

private:
    /**
     * @brief 初始化蓝牙组件
     */
    void initializeBluetooth();

    /**
     * @brief 处理接收到的原始数据
     * @param data 原始数据
     */
    void processReceivedData(const QByteArray& data);

    /**
     * @brief 获取命令字节数据
     * @param cmdType 命令类型
     * @return 命令字节数据
     */
    QByteArray getCommandBytes(CommandType cmdType);

private:
    // 蓝牙相关成员
    QBluetoothDeviceDiscoveryAgent* m_discoveryAgent;
    QLowEnergyController* m_controller;
    QLowEnergyService* m_service;
    QList<QBluetoothDeviceInfo> m_discoveredDevices;

    // 特征UUID
    QBluetoothUuid m_rxCharUuid;  // 接收特征(notify)
    QBluetoothUuid m_txCharUuid;  // 发送特征(write)

    // 状态管理
    ConnectionState m_connectionState;
    bool m_receivingData;

    // 数据缓冲
    QByteArray m_dataBuffer;
    static const int EXPECTED_LENGTH = 187;
    QMutex m_bufferMutex;

    // TCP服务器相关
    QTcpServer* m_tcpServer;
    QList<QTcpSocket*> m_tcpClients;
    bool m_receivingTcpData;

    // 数据处理
    QTimer* m_dataProcessTimer;
};

#endif // BLUETOOTHDEVICECONTROLLER_H
