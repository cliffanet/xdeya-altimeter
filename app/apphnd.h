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
    Q_OBJECT
    Q_PROPERTY(QString txtState READ getState NOTIFY stateChanged)
    Q_PROPERTY(PageSelector page READ getPage WRITE setPage NOTIFY pageChanged)
    Q_PROPERTY(bool progressEnabled READ getProgressEnabled NOTIFY progressChanged)
    Q_PROPERTY(int progressMax READ getProgressMax NOTIFY progressChanged)
    Q_PROPERTY(int progressVal READ getProgressVal NOTIFY progressChanged)
    Q_PROPERTY(int isInit READ isInit NOTIFY stateChanged)
    Q_PROPERTY(int isAuth READ isAuth NOTIFY stateChanged)
    Q_PROPERTY(bool reloadVisibled READ getReloadVisibled NOTIFY stateChanged)
    Q_PROPERTY(bool reloadEnabled READ getReloadEnabled NOTIFY stateChanged)
    Q_PROPERTY(QVariant devlist READ getDevList NOTIFY devListChanged)

public:
    enum PageSelector {
        PageDevSrch = 0,
        PageAuth,
        PageJmpList,
        PageJmpView,
        PageTrkView
    };
    Q_ENUM(PageSelector)

    NetProcess *netProc;

public:
    explicit AppHnd(QObject *parent = nullptr);

    const bool isDevSrch();
    const QString getState();

    void setErr(const QString &err);
    void clearErr();

    PageSelector getPage() const { return m_page; };
    void setPage(PageSelector page);
    Q_INVOKABLE void pageBack();

    const bool getProgressEnabled();
    const int getProgressMax();
    const int getProgressVal();
    bool isInit();
    bool isAuth();

    const bool getReloadVisibled();
    const bool getReloadEnabled();
    Q_INVOKABLE void clickReload();

    QVariant getDevList();
    Q_INVOKABLE void devConnect(qsizetype i);
    Q_INVOKABLE void devDisconnect();

    Q_INVOKABLE void authEdit(const QString &str);

signals:
    void stateChanged();
    void pagePushed(QString src);
    void pagePoped();
    void pageChanged();
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
    void netAuth(bool ok);
    void netData(quint32 pos, quint32 max);
    void netLogBook();
    void netTrack();
    void netTrkMapCenter(const log_item_t &ti);

private:
    QString m_err;
    PageSelector m_page;
    QList<PageSelector> m_pagehistory;

    QBluetoothDeviceDiscoveryAgent *btDAgent;
    WifiDeviceDiscovery *wfDAgent;
    QList<DevInfo *> m_devlist;
};

#endif // APPHND_H
