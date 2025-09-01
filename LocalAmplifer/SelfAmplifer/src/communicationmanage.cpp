#include "communicationmanage.h"
#include "Communication/tcpcommunication.h"
CommunicationManage::CommunicationManage(QObject *parent) : QObject(parent)
{

}
CommunicationManage::~CommunicationManage()
{
    delete communication;
}
Communication *CommunicationManage::setType(quint8 type)
{
    switch (type) {
        case 0:
            communication=new TCPCommunication;
        break;
    }
    return communication;
}
