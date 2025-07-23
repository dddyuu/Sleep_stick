// src/MainWindow/MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBluetoothDeviceInfo>
#include "Bluetooth/BluetoothDeviceController.h"
#include "Bluetooth/DataParser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onScanClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onStartRecvClicked();
    void onStopRecvClicked();
    void onSendTestCmdClicked();
    void onStartTcpServerClicked();
    void onStopTcpServerClicked();

    void onDeviceDiscovered(const QBluetoothDeviceInfo& device);
    void onConnectionStateChanged(BluetoothDeviceController::ConnectionState state);
    void onDataReceived(const DataParser::ParsedData& data);
    void onErrorOccurred(const QString& error);

private:
    void updateUIState();

private:
    Ui::MainWindow* ui;
    BluetoothDeviceController* controller;
};

#endif // MAINWINDOW_H