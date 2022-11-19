#include "apphnd.h"

#include "netprocess.h"
#include "devinfo.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QItemSelectionModel>

AppHnd::AppHnd(QObject *parent)
    : QObject{parent},
      m_page(pageDevSrch)
{
    netProc = new NetProcess(this);
    connect(netProc, &NetProcess::waitChange,   this, &AppHnd::netWait);
    connect(netProc, &NetProcess::rcvInit,      this, &AppHnd::netInit);
    connect(netProc, &NetProcess::rcvAuth,      this, &AppHnd::netAuth);
    connect(netProc, &NetProcess::rcvData,      this, &AppHnd::netData);
    connect(netProc, &NetProcess::rcvLogBook,   this, &AppHnd::netLogBook);
    connect(netProc, &NetProcess::rcvTrack,     this, &AppHnd::netTrack);
    connect(netProc->track(), &TrackHnd::onMapCenter, this, &AppHnd::netTrkMapCenter);

    btDAgent = new QBluetoothDeviceDiscoveryAgent(this);
    btDAgent->setLowEnergyDiscoveryTimeout(10000);
    qDebug() << btDAgent->supportedDiscoveryMethods();
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,    this, &AppHnd::btDiscovery);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,       this, &AppHnd::btError);
    connect(btDAgent, &QBluetoothDeviceDiscoveryAgent::finished,            this, &AppHnd::btDiscoverFinish);

    wfDAgent = new WifiDeviceDiscovery(this);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverDeviceFound,    this, &AppHnd::wfDiscovery);
    connect(wfDAgent, &WifiDeviceDiscovery::onError,                this, &AppHnd::wfError);
    connect(wfDAgent, &WifiDeviceDiscovery::discoverFinish,         this, &AppHnd::wfDiscoverFinish);
}

const bool AppHnd::isDevSrch()
{
    return btDAgent->isActive() || wfDAgent->isActive();
}

const QString AppHnd::getState()
{
    if (!m_err.isEmpty())
        return m_err;

    switch (netProc->wait()) {
        case NetProcess::wtDisconnected:
            switch (netProc->err()) {
                case NetProcess::errAuth:
                    return "Неверный код авторизации";

                case NetProcess::errProto:
                    return "Ошибка протокола";

                default:
                    return "Нет соединения";
            }

        case NetProcess::wtInit:
            return "Ожидание подключения";

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

void AppHnd::setPage(page_t page)
{
    if (page == m_page)
        return;
    m_page = page;

    emit progressChanged();
}

const bool AppHnd::getProgressEnabled()
{
    switch (m_page) {
        case pageDevSrch:
            return isDevSrch();

        default:
            return netProc->wait() > NetProcess::wtUnknown;
    }
}

const int AppHnd::getProgressMax()
{
    switch (m_page) {
        case pageDevSrch:
            return 0;

        default:
            switch (netProc->wait()) {
                case NetProcess::wtLogBook:
                case NetProcess::wtTrack:
                    return netProc->rcvMax();

                default:
                    return 0;
            }
    }
}

const int AppHnd::getProgressVal()
{
    switch (m_page) {
        case pageDevSrch:
            return 0;

        default:
            switch (netProc->wait()) {
                case NetProcess::wtLogBook:
                case NetProcess::wtTrack:
                    return netProc->rcvPos();

                default:
                    return 0;
            }
    }
}

const bool AppHnd::getReloadVisibled()
{
    return
        (m_page != pageJmpView) &&
        (m_page != pageTrkView);
}

const bool AppHnd::getReloadEnabled()
{
    switch (m_page) {
        case pageDevSrch:
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
        case pageDevSrch:
            m_devlist.clear();
            emit devListChanged();
            //btDAgent->start();
            wfDAgent->start();
            break;

        case pageJmpList:
            netProc->requestLogBook();
            break;

        case pageTrkView:
            //ui->wvTrack->reload();
            break;
    }

    emit stateChanged();
    emit progressChanged();
}

QVariant AppHnd::getDevList()
{
    return QVariant::fromValue(m_devlist);
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

void AppHnd::netInit()
{
    clearErr();
    /*
    FormAuth f;

    uint16_t code =
        f.exec() == QDialog::Accepted ?
                f.getCode() :
                0;

    if (code > 0) {
        netProc->requestAuth(code);
    }
    else {
        netProc->disconnect();
        ui->stackWnd->setCurrentIndex(pageDevSrch);
    }
    updState();
    */
}

void AppHnd::netAuth(bool ok)
{
    if (!ok)
        return;
    netProc->requestLogBook();
}

void AppHnd::netData(quint32 pos, quint32 max)
{
    emit progressChanged();
    /*
    ui->progRcvData->setRange(0, max);
    if (max > 0)
        ui->progRcvData->setValue(pos);

    switch (netProc->wait()) {
        case NetProcess::wtLogBook:
            emit mod_logbook->layoutChanged();
            ui->tvJmpList->scrollToBottom();
            break;
    }
    */
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
    //ui->wvTrack->page()->runJavaScript(cmd);
}

void AppHnd::netTrkMapCenter(const log_item_t &ti)
{
    /*
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

    ui->wvTrack->setHtml(data);
    */
}

