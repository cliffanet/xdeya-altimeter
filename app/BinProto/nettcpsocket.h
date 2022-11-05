#ifndef NETTCPSOCKET_H
#define NETTCPSOCKET_H

#include "netsocket.h"

class QTcpSocket;

class NetTcpSocket : public NetSocket
{
private:
    QTcpSocket *m_sock;
public:
    NetTcpSocket();
    NetTcpSocket(QTcpSocket *_sock);
    void setsock(QTcpSocket *_sock);
    QTcpSocket *sock() const { return m_sock; };
    void close();
    bool connected();
    size_t available();
    size_t recv(uint8_t *data, size_t sz);
    size_t send(const uint8_t *data, size_t sz);
};

#endif // NETTCPSOCKET_H
