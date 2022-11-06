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
    typedef enum {
        errNoError,
        errProto,
        errAuth
    } err_t;

    typedef enum {
        wtDisconnected = -1,
        wtUnknown = 0,
        wtInit,
        wtAuth
    } wait_t;

    explicit NetProcess(QObject *parent = nullptr);

    void connectTcp(const QHostAddress &ip, quint16 port);
    void disconnect();
    void resetAll();

    err_t err() const { return m_err; }
    wait_t wait() const { return m_wait; }
    bool connected() const { return m_pro.rcvstate() > BinProto::RCV_DISCONNECTED; }

    bool requestInit();
    bool requestAuth(uint16_t code);

signals:
    void waitChange(wait_t _wait);
    void rcvInit();
    void rcvAuth(bool isok);

private:
    err_t m_err;
    wait_t m_wait;
    BinProto m_pro;
    NetTcpSocket m_nettcp;
    QTcpSocket *tcpClient;

    void tcpConnected();
    void tcpDisconnected();
    void tcpError();

    void setWait(wait_t _wait);
    void rcvProcess();
    void rcvWrong();
};

#endif // NETPROCESS_H
