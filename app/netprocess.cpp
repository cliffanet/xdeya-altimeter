#include "netprocess.h"

#include <QTcpSocket>

NetProcess::NetProcess(QObject *parent)
    : QObject{parent}
{
    tcpClient = new QTcpSocket(this);
    connect(tcpClient, &QAbstractSocket::connected,     this, &NetProcess::tcpConnected);
    connect(tcpClient, &QAbstractSocket::disconnected,  this, &NetProcess::tcpDisconnected);
    connect(tcpClient, &QAbstractSocket::errorOccurred, this, &NetProcess::tcpError);
    connect(tcpClient, &QIODevice::readyRead,           this, &NetProcess::rcvProcess);
}

void NetProcess::connectTcp(const QHostAddress &ip, quint16 port)
{
    resetAll();
    tcpClient->connectToHost(ip, port);
}

void NetProcess::resetAll()
{
    tcpClient->close();
    m_nettcp.setsock(nullptr);
    m_pro.sock_clear();
}

void NetProcess::tcpConnected()
{
    m_nettcp.setsock(tcpClient);
    m_pro.sock_set(&m_nettcp);
    qDebug() << "TCP Socket connected to: " << tcpClient->peerAddress().toString() << ":" << tcpClient->peerPort();
}

void NetProcess::tcpDisconnected()
{
    if (m_nettcp.sock() == tcpClient) {
        m_nettcp.setsock(nullptr);
        if (m_pro.sock() == &m_nettcp)
            m_pro.sock_clear();
    }
    qDebug() << "TCP Socket disconnected";
}

void NetProcess::tcpError()
{
    qDebug() << "TCP Socket error: [" << tcpClient->error() << "] " << tcpClient->errorString();
}

void NetProcess::rcvProcess()
{
    m_pro.rcvprocess();
}
