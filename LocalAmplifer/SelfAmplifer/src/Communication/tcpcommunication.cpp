#include "tcpcommunication.h"
#include "QEventLoop"
#include "QMessageBox"
#include "QTimer"
#include "QThread"
TCPCommunication::TCPCommunication():socket(NULL)
{
    socket = NULL;
    server=new QTcpServer;
    udp=new QUdpSocket;
    connect(server,&QTcpServer::newConnection,this,&TCPCommunication::accept);
    //连接初始化
    connect_widget=new TCPConnectWidget;
    connect(connect_widget,&TCPConnectWidget::start,this,&TCPCommunication::listen);
    connect(connect_widget,&TCPConnectWidget::connection, this, &TCPCommunication::connection);
    connect(connect_widget,&TCPConnectWidget::close,this,&TCPCommunication::stop);
}

TCPCommunication::~TCPCommunication()
{

    if(socket!=NULL)
    {
        socket->deleteLater();
    }
    delete server;
    delete udp;
}

bool TCPCommunication::connectstatus()
{
    if(socket==NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void TCPCommunication::listen(quint16 port)
{
    server->listen(QHostAddress::Any,port);
    udp->bind(QHostAddress::Any,8080);
    emit newLogging("服务器开始监听");
}
void TCPCommunication::start()
{
    char data[3]={char(0xBB),char(0x66),char(0x01)};
    udp->writeDatagram(data,QHostAddress("192.168.1.104"),8080);
}

void TCPCommunication::stop()
{
    server->close();
    udp->close();
}

QByteArray TCPCommunication::readData()
{
    return socket->readAll();
}

void TCPCommunication::setParam(QString, QVariant value)
{
    connection("127.0.0.1", value.toString().toUInt());
}

QWidget *TCPCommunication::getConnectWidget()
{
    return connect_widget;
}

void TCPCommunication::connection(QString ip, quint16 port)
{
    if (socket == NULL)
    {
        delete socket;
    }
    socket = new QTcpSocket;
    socket->connectToHost(ip, port);
    qDebug() << "开始连接服务器";
    connect(socket, &QTcpSocket::readyRead, this, &TCPCommunication::readyRead);
    bool isok= connect(socket, &QTcpSocket::connected, this, [=]() {
        emit newLogging("服务器连接成功");
        emit connected();
    });
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(processError(QAbstractSocket::SocketError)));
    QMessageBox::about(NULL, "消息提示", "设备连接成功");

}

void TCPCommunication::accept()
{
    if(server->hasPendingConnections())
    {
        socket=server->nextPendingConnection();
        connect(socket,&QTcpSocket::readyRead,this,&TCPCommunication::readyRead);
        connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(processError(QAbstractSocket::SocketError)));
        emit newLogging("客户端连接");
        emit connected();
        start();
    }
}

void TCPCommunication::processError(QAbstractSocket::SocketError error)
{
    if(error==QAbstractSocket::RemoteHostClosedError)
    {
        socket->deleteLater();
        socket=NULL;
        emit newLogging("客户端断开");
    }
    QMessageBox::about(NULL, "", socket->errorString());
}
