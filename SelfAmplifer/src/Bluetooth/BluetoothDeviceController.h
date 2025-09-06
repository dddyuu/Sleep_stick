#ifndef BLUETOOTHDEVICECONTROLLER_H
#define BLUETOOTHDEVICECONTROLLER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptor>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include <QByteArray>
#include "DataParser.h"

class BluetoothDeviceController : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Disconnected,
        Scanning,
        Connecting,
        Connected
    };

    enum CommandType {
        StartCommand,
        StopCommand,
        StatusCommand
    };

    explicit BluetoothDeviceController(QObject* parent = nullptr);
    ~BluetoothDeviceController();

    void startScanning();
    void stopScanning();
    void connectToDevice(const QBluetoothDeviceInfo& device);
    void disconnectDevice();
    void startReceiving();
    void stopReceiving();
    void sendCommand(CommandType cmdType);
    void startTcpServer(quint16 port = 8888);
    void stopTcpServer();

    ConnectionState connectionState() const { return m_connectionState; }

signals:
    void deviceDiscovered(const QBluetoothDeviceInfo& device);
    void connectionStateChanged(ConnectionState state);
    void dataReceived(const DataParser::ParsedData& data);
    void errorOccurred(const QString& error);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo& device);
    void onScanFinished();
    void onControllerConnected();
    void onControllerDisconnected();
    void onServiceDiscovered(const QBluetoothUuid& serviceUuid);
    void onServiceDiscoveryFinished();  // 【新增】服务发现完成处理
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& characteristic,
        const QByteArray& newValue);
    // 【新增】添加更多服务操作的槽函数
    void onCharacteristicRead(const QLowEnergyCharacteristic& characteristic, const QByteArray& value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic& characteristic, const QByteArray& newValue);
    void onDescriptorRead(const QLowEnergyDescriptor& descriptor, const QByteArray& value);
    void onDescriptorWritten(const QLowEnergyDescriptor& descriptor, const QByteArray& newValue);
    void onNewTcpConnection();
    void onTcpDataReceived();
    void onTcpDisconnected();

private:
    void initializeBluetooth();
    void processReceivedData(const QByteArray& data);
    QByteArray getCommandBytes(CommandType cmdType);

    // 蓝牙相关
    QBluetoothDeviceDiscoveryAgent* m_discoveryAgent;
    QLowEnergyController* m_controller;
    QLowEnergyService* m_service;
    QList<QBluetoothDeviceInfo> m_discoveredDevices;

    // 特征UUID
    QBluetoothUuid m_rxCharUuid;
    QBluetoothUuid m_txCharUuid;

    // 连接状态
    ConnectionState m_connectionState;
    bool m_receivingData;

    // 数据处理
    QByteArray m_dataBuffer;
    QMutex m_bufferMutex;
    QTimer* m_dataProcessTimer;
    static const int EXPECTED_LENGTH = 187;

    // TCP服务器
    QTcpServer* m_tcpServer;
    QList<QTcpSocket*> m_tcpClients;
    bool m_receivingTcpData;
};

#endif // BLUETOOTHDEVICECONTROLLER_H