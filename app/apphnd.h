#ifndef APPHND_H
#define APPHND_H

#include <QObject>

#include "nettypes.h"
#include "wifidevicediscovery.h"
#include "jmpinfo.h"

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
    Q_PROPERTY(QVariant wifilist READ getWiFiList NOTIFY wifiListChanged)
    Q_PROPERTY(QVariant jmplist READ getJmpList NOTIFY jmpListChanged)
    Q_PROPERTY(QVariant trklist READ getTrkList NOTIFY trkListChanged)

public:
    enum PageSelector {
        PageDevSrch = 0,
        PageAuth,
        PageWiFiPass,
        PageJmpList,
        PageJmpInfo,
        PageTrkList,
        PageTrkView
    };
    Q_ENUM(PageSelector)

public:
    explicit AppHnd(QObject *parent = nullptr);

    bool isDevSrch() const;
    const QString getState() const;

    void setErr(const QString &err);
    void clearErr();

    PageSelector getPage() const { return m_page; };
    void setPage(PageSelector page);
    Q_INVOKABLE void pageBack();

    bool getProgressEnabled() const;
    int getProgressMax() const;
    int getProgressVal() const;
    bool isInit();
    bool isAuth();

    bool getReloadVisibled() const;
    bool getReloadEnabled() const;
    Q_INVOKABLE void clickReload();

    QVariant getDevList() const;
    Q_INVOKABLE void devConnect(qsizetype i);
    Q_INVOKABLE void devDisconnect();

    Q_INVOKABLE void authEdit(const QString &str);

    Q_INVOKABLE bool wifiPassLoad();
    QVariant getWiFiList();
    Q_INVOKABLE bool setWiFiSSID(qsizetype index, const QString ssid);
    Q_INVOKABLE bool setWiFiPass(qsizetype index, const QString pass);
    Q_INVOKABLE bool delWiFiPass(qsizetype index);
    Q_INVOKABLE void addWiFiPass();
    Q_INVOKABLE bool wifiPassSave();

    QVariant getJmpList() const;
    JmpInfo * getJmp() { return &m_jmp; }
    Q_INVOKABLE void setJmpInfo(int index);
    Q_INVOKABLE bool validJmpInfo(int index) const;

    QVariant getTrkList() const;
    Q_INVOKABLE void trkView(const trklist_item_t &trk);

signals:
    void stateChanged();
    void pagePushed(QString src);
    void pagePoped();
    void pageChanged();
    void progressChanged();
    void devListChanged();
    void wifiListChanged();
    void jmpListChanged();
    void jmpChanged();
    void jmpSelected(int index);
    void trkListChanged();
    void trkHtmlLoad(QString html);
    void trkRunJS(QString code);

    void cnfWiFiPass(uint8_t err);

private slots:
    void btDiscovery(const QBluetoothDeviceInfo &dev);
    void btError();
    void btDiscoverFinish();
    void wfDiscovery(const WifiDeviceItem &dev);
    void wfError(const QString &msg);
    void wfDiscoverFinish();

    void netWait();
    void netAuth(bool ok);
    void netCmdConfirm(uint8_t cmd, uint8_t err);
    void netData(quint32 pos, quint32 max);
    void netLogBook();
    void netTrack();
    void netTrkMapCenter(const log_item_t &ti);

    void jmpInfoChanged();

private:
    QString m_err;
    PageSelector m_page;
    QList<PageSelector> m_pagehistory;

    NetProcess *netProc;
    QBluetoothDeviceDiscoveryAgent *btDAgent;
    WifiDeviceDiscovery *wfDAgent;

    QList<DevInfo *> m_devlist;
    JmpInfo m_jmp;
};

#endif // APPHND_H
