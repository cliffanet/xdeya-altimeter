#ifndef NETPROCESS_H
#define NETPROCESS_H

#include <QObject>
#include "BinProto/nettcpsocket.h"
#include "BinProto/BinProto.h"

class QHostAddress;
class QTcpSocket;

class NetProcess : public QObject
{
    Q_OBJECT
public:
    explicit NetProcess(QObject *parent = nullptr);

    void connectTcp(const QHostAddress &ip, quint16 port);
    void resetAll();

signals:

private:
    BinProto m_pro;
    NetTcpSocket m_nettcp;
    QTcpSocket *tcpClient;

    void tcpConnected();
    void tcpDisconnected();
    void tcpError();

    void rcvProcess();
};

#endif // NETPROCESS_H
