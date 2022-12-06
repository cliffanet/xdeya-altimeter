#include "apphnd.h"

#include "netprocess.h"
#include "devinfo.h"
#include "wifiinfo.h"
#include "trkinfo.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QItemSelectionModel>
#include <QFile>

AppHnd::AppHnd(QObject *parent)
    : QObject{parent},
      m_page(PageDevSrch),
      m_jmp({ 0 })
{
    m_pagehistory.push_back(PageDevSrch);

    netProc = new NetProcess(this);
    connect(netProc, &NetProcess::waitChange,   this, &AppHnd::netWait);
    connect(netProc, &NetProcess::rcvAuth,      this, &AppHnd::netAuth);
    connect(netProc, &NetProcess::rcvCmdConfirm,this, &AppHnd::netCmdConfirm);
    connect(netProc, &NetProcess::rcvData,      this, &AppHnd::netData);
    connect(netProc, &NetProcess::rcvLogBook,   this, &AppHnd::netLogBook);
    connect(netProc, &NetProcess::rcvTrack,     this, &AppHnd::netTrack);
    connect(netProc->track(), &TrackHnd::onMapCenter, this, &AppHnd::netTrkMapCenter);

    btDAgent = new QBluetoothDeviceDiscoveryAgent(this);
    btDAgent->setLowEnergyDiscoveryTimeout(10000);
    qDebug() << btDAgent->supportedDiscoveryMethods();
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,    this, &AppHnd::btDiscovery);
#if QT_VERSION >= 0x060000
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,       this, &AppHnd::btError);
//#else
//    connect(btDAgent, SIGNAL(error),               this, SLOT(btError));
#endif
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::finished,            this, &AppHnd::btDiscoverFinish);

    wfDAgent = new WifiDeviceDiscovery(this);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverDeviceFound,    this, &AppHnd::wfDiscovery);
    connect(wfDAgent, &WifiDeviceDiscovery::onError,                this, &AppHnd::wfError);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverFinish,         this, &AppHnd::wfDiscoverFinish);

    connect(&m_jmp, &JmpInfo::changed, this, &AppHnd::jmpInfoChanged);
}

bool AppHnd::isDevSrch() const
{
    return btDAgent->isActive() || wfDAgent->isActive();
}

const QString AppHnd::getState() const
{
    if (!m_err.isEmpty())
        return m_err;

    switch (netProc->wait()) {
        case NetProcess::wtDisconnected:
            switch (netProc->err()) {
                case NetProcess::errSock:
                    return "Соединение разорвано";

                case NetProcess::errAuth:
                    return "Неверный код авторизации";

                case NetProcess::errProto:
                    return "Ошибка протокола";

                default:
                    return "Нет соединения";
            }

        case NetProcess::wtConnecting:
        case NetProcess::wtInit:
            return "Ожидание подключения";

        //case NetProcess::wtAuthReq:
        case NetProcess::wtAuth:
            return "Идентификация";

        case NetProcess::wtWiFiBeg:
        case NetProcess::wtWiFi:
            return "Приём wifi-паролей";

        case NetProcess::wtWiFiSend:
            return "Сохранение wifi-паролей";

        case NetProcess::wtLogBookBeg:
        case NetProcess::wtLogBook:
            return "Приём логбука";

        case NetProcess::wtTrkListBeg:
        case NetProcess::wtTrkList:
            return "Приём списка треков";

        case NetProcess::wtTrackBeg:
        case NetProcess::wtTrack:
            return "Приём трека";

        default:
            return "";
    }
}

void AppHnd::setErr(const QString &err)
{
    if (err == m_err)
        return;
    m_err = err;
    emit stateChanged();
}

void AppHnd::clearErr()
{
    setErr("");
}

void AppHnd::setPage(PageSelector page)
{
    qDebug() << "set page: " + QString::number(page);
    if (page == m_page)
        return;
    m_page = page;

    int n = 0;
    for (const auto p: m_pagehistory) {
        n++;
        if (p == page) break;
    }

    if (n > 0)
        while (n > m_pagehistory.size()) {
            m_pagehistory.pop_back();
            emit pagePoped();
        }

    if (m_pagehistory.back() != page)
        switch (page) {
            case PageAuth:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/auth.qml");
                break;
            case PageWiFiPass:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/wifipass.qml");
                break;
            case PageJmpList:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/jmplist.qml");
                break;
            case PageJmpInfo:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/jmpinfo.qml");
                break;
            case PageTrkList:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/trklist.qml");
                break;
            case PageTrkView:
                m_pagehistory.push_back(page);
                emit pagePushed("qrc:/page/trkview.qml");
                break;
            default:
                break;
        }

    emit pageChanged();
    emit stateChanged();
    emit progressChanged();
}

void AppHnd::pageBack()
{
    if (m_pagehistory.size() <= 1)
        return;
    m_pagehistory.pop_back();
    emit pagePoped();

    setPage(m_pagehistory.back());
}

bool AppHnd::getProgressEnabled() const
{
    switch (m_page) {
        case PageDevSrch:
            return isDevSrch();

        default:
            return netProc->wait() > NetProcess::wtUnknown;
    }
}

int AppHnd::getProgressMax() const
{
    switch (m_page) {
        case PageDevSrch:
            return 0;

        default:
            switch (netProc->wait()) {
                case NetProcess::wtWiFi:
                case NetProcess::wtLogBook:
                case NetProcess::wtTrack:
                    return netProc->rcvMax();

                default:
                    return 0;
            }
    }
}

int AppHnd::getProgressVal() const
{
    switch (m_page) {
        case PageDevSrch:
            return 0;

        default:
            switch (netProc->wait()) {
                case NetProcess::wtWiFi:
                case NetProcess::wtLogBook:
                case NetProcess::wtTrack:
                    return netProc->rcvPos();

                default:
                    return 0;
            }
    }
}

bool AppHnd::isInit()
{
    return
        (netProc->wait() == NetProcess::wtConnecting) ||
        (netProc->wait() == NetProcess::wtInit) ||
        (netProc->wait() == NetProcess::wtAuth);
}

bool AppHnd::isAuth()
{
    return netProc->wait() == NetProcess::wtAuthReq;
}

bool AppHnd::getReloadVisibled() const
{
    return
        (m_page != PageAuth) &&
        (m_page != PageJmpInfo) &&
        (m_page != PageTrkView);
}

bool AppHnd::getReloadEnabled() const
{
    switch (m_page) {
        case PageDevSrch:
            return !isDevSrch();

        default:
            return netProc->wait() == NetProcess::wtUnknown;
    }
}

void AppHnd::clickReload()
{
    qDebug() << "reload clicked";

    clearErr();

    switch (m_page) {
        case PageDevSrch:
            m_devlist.clear();
            emit devListChanged();
            //btDAgent->start();
            wfDAgent->start();
            break;

        case PageWiFiPass:
            netProc->requestWiFiPass();
            emit wifiListChanged();
            break;

        case PageJmpList:
            netProc->requestLogBook();
            emit jmpListChanged();
            break;

        case PageTrkList:
            netProc->requestTrackList();
            emit trkListChanged();
            break;

        case PageTrkView:
            //ui->wvTrack->reload();
            break;

        default:
            break;
    }

    emit stateChanged();
    emit progressChanged();
}

QVariant AppHnd::getDevList() const
{
    return QVariant::fromValue(m_devlist);
}

void AppHnd::devConnect(int i)
{
    if ((i < 0) || (i >= m_devlist.size()))
        return;

    if (btDAgent->isActive())
        btDAgent->stop();
    if (wfDAgent->isActive())
        wfDAgent->stop();

    clearErr();

    const auto dev = m_devlist[i];

    qDebug() << "Connect to: " << dev->getName() << " (" << dev->getAddress() << ")";
    switch (dev->src()) {
        case DevInfo::SRC_WIFI: {
            auto &wifi = dev->wifi();
            netProc->connectTcp(wifi.ip, wifi.port);
            break;
        }

        default:
            return;
    }

    emit stateChanged();
    emit progressChanged();
}

void AppHnd::devDisconnect()
{
    netProc->disconnect();
}

void AppHnd::authEdit(const QString &str)
{
    qDebug() << "edit: " << str;
    if (str.length() != 4)
        return;

    bool ok = false;
    uint16_t code = str.toUShort(&ok, 16);
    if (!ok)
        return;

    netProc->requestAuth(code);

}

bool AppHnd::wifiPassLoad()
{
    clearErr();
    return netProc->requestWiFiPass();
}

QVariant AppHnd::getWiFiList()
{
    return QVariant::fromValue(netProc->wifipass());
}

bool AppHnd::setWiFiSSID(qsizetype index, const QString ssid)
{
    if ((index < 0) || (index > netProc->wifipass().size()))
        return false;

    auto &winf = netProc->wifipass()[index];
    winf->setSSID(ssid);

    return true;
}

bool AppHnd::setWiFiPass(qsizetype index, const QString pass)
{
    if ((index < 0) || (index > netProc->wifipass().size()))
        return false;

    auto &winf = netProc->wifipass()[index];
    winf->setPass(pass);

    return true;
}

bool AppHnd::delWiFiPass(qsizetype index)
{
    if ((index < 0) || (index > netProc->wifipass().size()))
        return false;

    netProc->wifipass().removeAt(index);
    emit wifiListChanged();

    return true;
}

void AppHnd::addWiFiPass()
{
    netProc->wifipass().push_back(new WiFiInfo());
    emit wifiListChanged();
}

bool AppHnd::wifiPassSave()
{
    clearErr();
    return netProc->sendWiFiPass();
}

QVariant AppHnd::getJmpList() const
{
    return QVariant::fromValue(netProc->logbook());
}

void AppHnd::setJmpInfo(int index)
{
    clearErr();
    if ((index < 0) || (index >= netProc->logbook().size()))
        m_jmp.clear();
    else {
        m_jmp.set(netProc->logbook()[index], index);
        m_jmp.fetchTrkList(netProc->trklist());
        emit jmpSelected(index);
    }
}

bool AppHnd::validJmpInfo(int index) const
{
    return (index >= 0) && (index < netProc->logbook().size());
}

QVariant AppHnd::getTrkList() const
{
    return QVariant::fromValue(netProc->trklist());
}

void AppHnd::trkView(int index)
{
    clearErr();

    emit trkHtmlLoad("");
    if ((index < 0) || (index >= netProc->trklist().size()))
        return;

    netProc->requestTrack(netProc->trklist()[index]->trk());
}


void AppHnd::btDiscovery(const QBluetoothDeviceInfo &dev)
{
    qDebug() << "BT Found new device:" << dev.name() << '(' << dev.address().toString() << ')';
    if (
            dev.isValid() &&
            !dev.address().isNull() &&
            (dev.name() != tr(""))
            //(dev.address().toString() != tr("00:00:00:00:00:00") )
        )
        m_devlist.append(new DevInfo(dev));
        emit devListChanged();
}

void AppHnd::btError()
{
    setErr(btDAgent->errorString());
    emit progressChanged();
}

void AppHnd::btDiscoverFinish()
{
    qDebug() << "BT Found finish()";
    emit stateChanged();
    emit progressChanged();
}

void AppHnd::wfDiscovery(const WifiDeviceItem &dev)
{
    qDebug() << "Wifi Found new device:" << dev.name << '(' << dev.ip.toString() << ':' << dev.port << ')';
    m_devlist.append(new DevInfo(dev));
    emit devListChanged();
}

void AppHnd::wfError(const QString &msg)
{
    setErr( msg );
    emit progressChanged();
}

void AppHnd::wfDiscoverFinish()
{
    qDebug() << "Wifi Found finish()";
    emit stateChanged();
    emit progressChanged();
}

void AppHnd::netWait()
{
    emit stateChanged();
    emit progressChanged();
}

void AppHnd::netAuth(bool ok)
{
    if (!ok)
        return;
    netProc->requestLogBook();
    pageBack();
    setPage(PageJmpList);
}

void AppHnd::netCmdConfirm(uint8_t cmd, uint8_t err)
{
    switch (cmd) {
        case 0x41:
            emit cnfWiFiPass(err);
            if (err == 0) {
                if (m_page == PageWiFiPass)
                    pageBack();
            }
            else
                setErr("Ошибка сохранения");
            break;
    }
}

void AppHnd::netData(quint32 pos, quint32 max)
{
    Q_UNUSED(pos);
    Q_UNUSED(max);
    emit progressChanged();

    switch (netProc->wait()) {
        case NetProcess::wtLogBook:
            emit jmpListChanged();
            break;
        case NetProcess::wtWiFi:
            emit wifiListChanged();
            break;
        case NetProcess::wtTrkList:
            emit trkListChanged();
            break;
        default:
            break;
    }
}

void AppHnd::netLogBook()
{
    clearErr();
    netProc->requestTrackList();
}

void AppHnd::netTrack()
{
    if (!netProc->track()->mapCenter()) {
        setErr("Нет связи со спутниками");
        return;
    }

    QByteArray json;
    netProc->track()->saveGeoJson(json);
    QString cmd(QString("loadGPX(")+json.constData()+");");

    emit trkRunJS(cmd);
}

void AppHnd::netTrkMapCenter(const log_item_t &ti)
{
    QString center(QString::number(static_cast<double>(ti.lat)/10000000) + tr(", ") + QString::number(static_cast<double>(ti.lon)/10000000));
    qDebug() << "netTrkMapCenter: " << center;

    QString data;
    QFile ftxt(QString(":/navi.html"));
    if (!ftxt.open(QIODevice::ReadOnly)) {
        qDebug() << "file(" << ftxt.fileName() << ") not opened";
        return;
    }
    data = ftxt.readAll();
    ftxt.close();
    data.replace(QString("%MAP_CENTER%"), center);

    emit trkHtmlLoad(data);
}

void AppHnd::jmpInfoChanged()
{
    emit jmpChanged();
}

