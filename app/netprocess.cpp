#include "netprocess.h"

#include <QTcpSocket>

NetProcess::NetProcess(QObject *parent)
    : QObject{parent},
      m_err(errNoError),
      m_wait(wtDisconnected)
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

void NetProcess::disconnect()
{
    auto *sock = m_pro.sock();
    if (sock == nullptr)
        return;
    sock->close();
}

void NetProcess::resetAll()
{
    m_pro.sock_clear();
    m_nettcp.setsock(nullptr);
    if (tcpClient->isOpen())
        tcpClient->close();
    setWait(wtDisconnected);
    m_err = errNoError;
}

bool NetProcess::requestInit()
{
    if (m_wait != wtUnknown)
        return false;
    if (!m_pro.send(0x02))
        return false;
    setWait(wtInit);
    return true;
}

bool NetProcess::requestAuth(uint16_t code)
{
    if (m_wait != wtUnknown)
        return false;
    if (code == 0)
        return false;
    if (!m_pro.send(0x03, "n", code))
        return false;
    setWait(wtAuth);
    return true;
}

void NetProcess::tcpConnected()
{
    m_nettcp.setsock(tcpClient);
    m_pro.sock_set(&m_nettcp);
    setWait(wtUnknown);

    qDebug() << "TCP Socket connected to: " << tcpClient->peerAddress().toString() << ":" << tcpClient->peerPort();

    requestInit();
}

void NetProcess::tcpDisconnected()
{
    qDebug() << "TCP Socket disconnected";
    rcvProcess();
}

void NetProcess::tcpError()
{
    qDebug() << "TCP Socket error: [" << tcpClient->error() << "] " << tcpClient->errorString();
}

void NetProcess::setWait(wait_t _wait)
{
    if (_wait == m_wait)
        return;
    m_wait = _wait;
    if (_wait > wtDisconnected)
        m_err = errNoError;
    emit waitChange(_wait);
}

void NetProcess::rcvProcess()
{
    m_pro.rcvprocess();
    while (m_pro.rcvstate() == BinProto::RCV_COMPLETE) {
        //if (m_wait <= wtUnknown) {
        //    m_pro.rcvnext();
        //    continue;
        //}

        // Любую команду надо сначала принять:
        // т.е. выполнить либо rcvdata(), либо rcvnext().
        // И только после этого заниматься обработкой этой команды.
        // Иначе, любой emit (сигнал), может привести к тому,
        // что в обратку придёт вызов сюда же, и будет повторный приём этой же команды,
        // т.к. её приём ещё не завершён, обработка в m_pro текущей команды ещё не закрыта.

        switch (m_pro.rcvcmd()) {
            case 0x02: // hello -> надо отправить requestAuth()
                if (m_wait != wtInit)
                    return rcvWrong();
                m_pro.rcvnext(); // эта команда без данных
                setWait(wtUnknown);
                emit rcvInit();
                break;

        case 0x03: { // auth result
            if (m_wait != wtAuth)
                return rcvWrong();
            uint8_t err = 0;
            m_pro.rcvdata("C", err);
            qDebug() << "Auth err: " << err;
            if (err > 0) {
                m_err = errAuth;
                m_pro.sock()->close();
            }
            else
                setWait(wtUnknown);
            emit rcvAuth(err == 0);
            break;
        }

            default:
                return rcvWrong();
        }

    }

    if (m_pro.rcvstate() <= BinProto::RCV_DISCONNECTED) {
        if (m_pro.sock() == &m_nettcp)
            m_nettcp.setsock(nullptr);
        m_pro.sock_clear();
        setWait(wtDisconnected);
        if (m_pro.rcvstate() == BinProto::RCV_ERROR)
            m_err = errProto;
    }
}

void NetProcess::rcvWrong()
{
    char s[1024];
    snprintf(s, sizeof(s), "cmd: 0x%02x [len: %d]; wait = %d", m_pro.rcvcmd(), m_pro.rcvsz(), m_wait);
    qDebug() << "Recv unknown cmd: " << s;

    m_pro.rcvnext();
    setWait(wtUnknown);
}
