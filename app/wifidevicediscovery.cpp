#include "wifidevicediscovery.h"

#include <QTimer>
#include <QUdpSocket>

WifiDeviceDiscovery::WifiDeviceDiscovery(QObject *parent)
    : QObject{parent},
      m_discoverytimeout(15000)
{
    m_sock = new QUdpSocket(this);
    connect(m_sock, &QUdpSocket::readyRead, this, &WifiDeviceDiscovery::readyRead);

    tmrDiscovery = new QTimer(this);
    connect(tmrDiscovery, &QTimer::timeout, this, &WifiDeviceDiscovery::stop);
}

void WifiDeviceDiscovery::setDiscoveryTimeout(int timeout)
{
    m_discoverytimeout = timeout;
}

void WifiDeviceDiscovery::start()
{
    m_list.clear();

    m_sock->bind(QHostAddress::AnyIPv4, 3310);

    if (m_discoverytimeout > 0)
        tmrDiscovery->start(m_discoverytimeout);

    emit discoverBegin();
}

void WifiDeviceDiscovery::stop()
{
    if (m_sock->state() == QAbstractSocket::BoundState)
        m_sock->close();
    if (tmrDiscovery->isActive())
        tmrDiscovery->stop();

    emit discoverFinish();
}

bool WifiDeviceDiscovery::isActive() const
{
     return (m_sock != nullptr) && (m_sock->state() == QAbstractSocket::BoundState);
}

void WifiDeviceDiscovery::readyRead()
{
    while (m_sock->hasPendingDatagrams()) {
        QHostAddress ip;
        quint16 port;
        char buf[1024];
        qint64 sz = m_sock->readDatagram(buf, sizeof(buf)-1, &ip);
        if (sz < 7)
            continue;
        buf[sz] = '\0';
        if (memcmp(buf+2, "XdeYa", 5) != 0)
            continue;
        memcpy(&port, buf, sizeof(port));
        port = ntohs(port);

        bool found = false;
        for (auto &d: m_list)
            if ((d.ip == ip) && (d.port == port)) {
                found = true;
                d.name = QString(buf+2);
                break;
            }

        if (!found) {
            WifiDeviceItem dev = {
                .name   = QString(buf+2),
                .ip     = ip,
                .port   = port
            };
            m_list.push_back(dev);
            emit discoverDeviceFound(dev);
        }
    }
}
