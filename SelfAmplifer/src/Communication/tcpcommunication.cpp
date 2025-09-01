#include "tcpcommunication.h"

TCPCommunication::TCPCommunication():socket(NULL)
{
    socket = NULL;
    server=new QTcpServer;
    udp=new QUdpSocket;
    connect(server,&QTcpServer::newConnection,this,&TCPCommunication::accept);
    //连接初始化
    connect_widget=new TCPConnectWidget;
    connect(connect_widget,&TCPConnectWidget::start,this,&TCPCommunication::listen);
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
    //emit newLogging("hellocket");
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
    udp->writeDatagram(data,QHostAddress("255.255.255.255"),8080);
}

void TCPCommunication::stop()
{
    server->close();
    udp->close();
}

QWidget *TCPCommunication::getConnectWidget()
{
    return connect_widget;
}

void TCPCommunication::accept()
{
    if(server->hasPendingConnections())
    {
        socket=server->nextPendingConnection();
        connect(socket,&QTcpSocket::readyRead,this,&TCPCommunication::readData);
        connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(processError(QAbstractSocket::SocketError)));
        emit newLogging("客户端连接");
        emit connected();
        start();
    }
}

void TCPCommunication::readData()
{
    QByteArray data=socket->readAll();
    emit readyRead(data);
}

void TCPCommunication::processError(QAbstractSocket::SocketError error)
{
    if(error==QAbstractSocket::RemoteHostClosedError)
    {
        socket->deleteLater();
        socket=NULL;
        emit newLogging("客户端断开");
    }
}
