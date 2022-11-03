#ifndef WIFIDEVICEDISCOVERY_H
#define WIFIDEVICEDISCOVERY_H

#include <QObject>

class QTimer;
class QUdpSocket;

#include <QHostAddress>

typedef struct {
    QString name;
    QHostAddress ip;
    quint16 port;
} WifiDeviceItem;

class WifiDeviceDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit WifiDeviceDiscovery(QObject *parent = nullptr);

    int getDiscoveryTimeout() const { return m_discoverytimeout; }
    void setDiscoveryTimeout(int timeout);

    void start();
    void stop();
    bool isActive() const;

signals:
    void discoverBegin();
    void discoverDeviceFound(const WifiDeviceItem &dev);
    void discoverFinish();
    void onError(const QString &msg);

private:
    int m_discoverytimeout;
    QUdpSocket *m_sock;
    QTimer *tmrDiscovery;
    QList<WifiDeviceItem> m_list;

    void readyRead();
};

#endif // WIFIDEVICEDISCOVERY_H
