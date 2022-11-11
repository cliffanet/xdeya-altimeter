#include "netprocess.h"

#include <QTcpSocket>
#include <QHttpServer>
#include <QTcpServer>

static logbook_item_t lb_item_null = {};
static trklist_item_t tl_item_null = {};

NetProcess::NetProcess(QObject *parent)
    : QObject{parent},
      m_err(errNoError),
      m_wait(wtDisconnected),
      m_rcvpos(0), m_rcvcnt(0),
      m_trkmapcenter(false)
{
    tcpClient = new QTcpSocket(this);
    connect(tcpClient, &QAbstractSocket::connected,     this, &NetProcess::tcpConnected);
    connect(tcpClient, &QAbstractSocket::disconnected,  this, &NetProcess::tcpDisconnected);
    connect(tcpClient, &QAbstractSocket::errorOccurred, this, &NetProcess::tcpError);
    connect(tcpClient, &QIODevice::readyRead,           this, &NetProcess::rcvProcess);
    m_http = new QHttpServer(this);
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
    m_rcvpos = 0;
    m_rcvcnt = 0;
    m_http->servers().clear();
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

bool NetProcess::requestLogBook(uint32_t beg, uint32_t count)
{
    if (m_wait != wtUnknown)
        return false;

    struct {
        uint32_t beg;
        uint32_t count;
    } data = { beg, count };
    if (!m_pro.send(0x31, "NN", data))
        return false;
    setWait(wtLogBookBeg);
    return true;
}

bool NetProcess::requestTrackList()
{
    if (m_wait != wtUnknown)
        return false;

    if (!m_pro.send(0x51))
        return false;
    setWait(wtTrkListBeg);
    return true;
}

bool NetProcess::requestTrack(quint32 i)
{
    if (i >= m_trklist.size())
        return false;
    return requestTrack(m_trklist[i]);
}

bool NetProcess::requestTrack(const trklist_item_t &trk)
{
    if (m_wait != wtUnknown)
        return false;

    trksrch_t ts = {
        .id     = trk.id,
        .jmpnum = trk.jmpnum,
        .jmpkey = trk.jmpkey,
        .tmbeg  = trk.tmbeg,
        .fnum   = trk.fnum
    };

    if (!m_pro.send(0x54, "NNNTC", ts))
        return false;
    setWait(wtTrackBeg);
    return true;
}

const logbook_item_t &NetProcess::logbook(quint32 i) const
{
    if (i >= m_logbook.size())
        return lb_item_null;
    return m_logbook[i];
}

const trklist_item_t &NetProcess::trklist(quint32 i) const
{
    if (i >= m_trklist.size())
        return tl_item_null;
    return m_trklist[i];
}

bool NetProcess::trkSaveGPX(QIODevice &fh)
{
    if (!fh.isOpen() || !fh.isWritable())
        return false;

#define FSTR(len, str) if (fh.write( str ) < len) return false

    FSTR(100,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<gpx version=\"1.1\" creator=\"XdeYa\" xmlns=\"http://www.topografix.com/GPX/1/1\">"
            "<metadata>"
                "<name><![CDATA[Без названия]]></name>"
                "<desc/>"
                "<time>2017-10-18T12:19:23.353Z</time>"
            "</metadata>"
            "<trk>"
                    "<name>трек</name>"
    );

    bool prevok = false;
    char s[1024];
#define FFMT(fmt, ...)  \
        do { \
            snprintf(s, sizeof(s), fmt, ##__VA_ARGS__); \
            FSTR(5, s); \
        } while (0)

    for (const auto &p : m_track) {
        bool isok = (p.flags & 0x0001) > 0;
        if (prevok != isok) {
            FSTR(5, isok ? "<trkseg>" : "</trkseg>");
            prevok = isok;
        }
        if (!isok)
            continue;

        FFMT("<trkpt lat=\"%0.6f\" lon=\"%0.6f\">",
             static_cast<double>(p.lat)/10000000,
             static_cast<double>(p.lon)/10000000);

        uint32_t sec = p.tmoffset / 1000;
        uint32_t min = sec / 60;
        sec -= min*60;
        FFMT("<name>%d:%02d, %d m / %0.1f m/s</name>",
             min, sec, p.alt, static_cast<double>(p.altspeed)/100
        );

        FFMT("<desc>Горизонт: %d&deg; / %0.1f m/s", p.heading, static_cast<double>(p.hspeed)/100);
        if (p.altspeed > 0) {
            FFMT(" (кач: %0.1f)", -1.0 * p.hspeed / p.altspeed);
        }
        FFMT("</desc>");

        FFMT("<ele>%d</ele>", p.alt);
        FFMT("<magvar>%d</magvar>", p.heading);
        FFMT("<sat>%d</sat>", p.sat);

        FFMT("</trkpt>");
    }

    if (prevok)
        FSTR(5, "</trkseg>");

    FSTR(10,
            "</trk>"
        "</gpx>"
    );

#undef FSTR
#undef FFMT

    return true;
}

QString NetProcess::httpAddr()
{
    if (m_http->servers().count() == 0)
        return "";

    return "http://" + QHostAddress(QHostAddress::LocalHost).toString() + ":" + QString::number(m_http->servers()[0]->serverPort());
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

            case 0x31: { // logbook begin
                if (m_wait != wtLogBookBeg)
                    return rcvWrong();
                struct { uint32_t beg, count; } d;
                m_pro.rcvdata("NN", d);
                qDebug() << "Log book beg: " << d.beg << " / " << d.count;
                m_rcvpos = 0;
                m_rcvcnt = d.count;
                m_logbook.clear();
                setWait(wtLogBook);
                break;
            }

            case 0x32: { // logbook item
                if (m_wait != wtLogBook)
                    return rcvWrong();
                logbook_item_t d;
                m_pro.rcvdata("NNT" LOG_PK LOG_PK LOG_PK LOG_PK, d);
                m_logbook.push_back(d);
                m_rcvpos ++;
                emit rcvData(m_rcvpos, m_rcvcnt);
                break;
            }

            case 0x33: { // logbook end
                if (m_wait != wtLogBook)
                    return rcvWrong();
                m_pro.rcvnext(); // данные у команды есть, но нам они не нужны
                setWait(wtUnknown);
                emit rcvLogBook();
                break;
            }

            case 0x51: { // tracklist begin
                if (m_wait != wtTrkListBeg)
                    return rcvWrong();
                m_pro.rcvnext(); // данных у команды нет
                m_rcvpos = 0;
                m_rcvcnt = 0;
                m_trklist.clear();
                setWait(wtTrkList);
                break;
            }

            case 0x52: { // tracklist item
                if (m_wait != wtTrkList)
                    return rcvWrong();
                trklist_item_t d;
                m_pro.rcvdata("NNNNTNC", d);
                m_trklist.push_back(d);
                m_rcvpos ++;
                emit rcvData(m_rcvpos, m_rcvcnt);
                break;
            }

            case 0x53: { // tracklist end
                if (m_wait != wtTrkList)
                    return rcvWrong();
                m_pro.rcvnext(); // данных у команды нет
                setWait(wtUnknown);
                emit rcvTrkList();
                break;
            }

            case 0x54: { // trackdata begin
                if (m_wait != wtTrackBeg)
                    return rcvWrong();
                m_pro.rcvdata("NNNNTNH", m_trkinfo);
                m_rcvpos = 0;
                m_rcvcnt = m_trkinfo.fsize;
                m_track.clear();
                m_trkmapcenter = false;
                setWait(wtTrack);
                break;
            }

            case 0x55: { // trackdata item
                if (m_wait != wtTrack)
                    return rcvWrong();
                log_item_t ti;
                m_pro.rcvdata(LOG_PK, ti);
                m_track.push_back(ti);
                m_rcvpos += sizeof(log_item_t);
                emit rcvData(m_rcvpos, m_rcvcnt);
                if (!m_trkmapcenter && ((ti.flags & 0x0001) > 0)) {
                    m_trkmapcenter = true;
                    emit rcvTrkMapCenter(ti);
                }
                break;
            }

            case 0x56: { // trackdata end
                if (m_wait != wtTrack)
                    return rcvWrong();
                m_pro.rcvnext(); // данных у команды нет
                setWait(wtUnknown);
                if (m_http->servers().count() == 0)
                    httpInit();
                emit rcvTrack(m_trkinfo);
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

void NetProcess::httpInit()
{
    m_http->route("/track.gpx", [] () {
        qDebug() << "request track";
        return "hello world";
    });
    m_http->listen(QHostAddress::LocalHost);
    qDebug() << "server init: " << m_http->servers().count();
}
