#ifndef APPHND_H
#define APPHND_H

#include <QObject>

#include "nettypes.h"
#include "wifidevicediscovery.h"

#include <QList>
#include <QVariant>

class NetProcess;
class QBluetoothDeviceDiscoveryAgent;
class QBluetoothDeviceInfo;

class DevInfo;

class AppHnd : public QObject
{
    typedef enum {
        pageDevSrch = 0,
        pageJmpList,
        pageJmpView,
        pageTrkView
    } page_t;

    Q_OBJECT
    Q_PROPERTY(QString txtState READ getState NOTIFY stateChanged)
    Q_PROPERTY(bool progressEnabled READ getProgressEnabled NOTIFY progressChanged)
    Q_PROPERTY(int progressMax READ getProgressMax NOTIFY progressChanged)
    Q_PROPERTY(int progressVal READ getProgressVal NOTIFY progressChanged)
    Q_PROPERTY(bool reloadVisibled READ getReloadVisibled NOTIFY stateChanged)
    Q_PROPERTY(bool reloadEnabled READ getReloadEnabled NOTIFY stateChanged)
    Q_PROPERTY(QVariant devlist READ getDevList NOTIFY devListChanged)

public:
    NetProcess *netProc;

public:
    explicit AppHnd(QObject *parent = nullptr);

    const bool isDevSrch();
    const QString getState();

    void setErr(const QString &err);
    void clearErr();

    void setPage(page_t page);

    const bool getProgressEnabled();
    const int getProgressMax();
    const int getProgressVal();

    const bool getReloadVisibled();
    const bool getReloadEnabled();
    Q_INVOKABLE void clickReload();

    QVariant getDevList();

signals:
    void stateChanged();
    void progressChanged();
    void devListChanged();

private slots:
    void btDiscovery(const QBluetoothDeviceInfo &dev);
    void btError();
    void btDiscoverFinish();
    void wfDiscovery(const WifiDeviceItem &dev);
    void wfError(const QString &msg);
    void wfDiscoverFinish();

    void netWait();
    void netInit();
    void netAuth(bool ok);
    void netData(quint32 pos, quint32 max);
    void netLogBook();
    void netTrack();
    void netTrkMapCenter(const log_item_t &ti);

private:
    QString m_err;
    page_t m_page;

    QBluetoothDeviceDiscoveryAgent *btDAgent;
    WifiDeviceDiscovery *wfDAgent;
    QList<DevInfo *> m_devlist;
};

#endif // APPHND_H
