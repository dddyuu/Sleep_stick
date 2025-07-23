#ifndef TCPCOMMUNICATION_H
#define TCPCOMMUNICATION_H

#include "communication.h"
#include "QTcpServer"
#include "QTcpSocket"
#include "QUdpSocket"
#include "tcpconnectwidget.h"
class TCPCommunication:public Communication
{
    Q_OBJECT
public:
    TCPCommunication();
    ~TCPCommunication();
    bool connectstatus() override;
    QWidget *getConnectWidget() override;
    QByteArray readData() override;
    virtual void setParam(QString, QVariant);
private slots:
    void listen(quint16);
    void connection(QString, quint16);
    void accept();
    void processError(QAbstractSocket::SocketError);
private:
    QTcpServer *server;
    QTcpSocket *socket;
    QUdpSocket *udp;
    TCPConnectWidget *connect_widget;
    void start();
    void stop();
};

#endif // TCPCOMMUNICATION_H
