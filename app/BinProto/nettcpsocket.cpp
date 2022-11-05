#include "nettcpsocket.h"
#include <QTcpSocket>

NetTcpSocket::NetTcpSocket() :
    m_sock(NULL)
{

}

NetTcpSocket::NetTcpSocket(QTcpSocket *_sock) :
    m_sock(_sock)
{

}

void NetTcpSocket::setsock(QTcpSocket *_sock)
{
    m_sock = _sock;
}

void NetTcpSocket::close()
{
    if (m_sock == NULL)
        return;
    m_sock->close();
}

bool NetTcpSocket::connected()
{
    if (m_sock == NULL)
        return false;
    return m_sock->state() == QAbstractSocket::ConnectedState;
}

size_t NetTcpSocket::available()
{
    if (m_sock == NULL)
        return 0;
    return m_sock->bytesAvailable();
}

size_t NetTcpSocket::recv(uint8_t *data, size_t sz)
{
    if (m_sock == NULL)
        return -1;
    return m_sock->read(reinterpret_cast<char *>(data), sz);
}

size_t NetTcpSocket::send(const uint8_t *data, size_t sz)
{
    if (m_sock == NULL)
        return -1;
    return m_sock->write(reinterpret_cast<const char *>(data), sz);
}
