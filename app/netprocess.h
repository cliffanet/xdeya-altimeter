#ifndef NETPROCESS_H
#define NETPROCESS_H

#include <QObject>
#include "BinProto/nettcpsocket.h"
#include "BinProto/BinProto.h"
#include "nettypes.h"
#include "trackhnd.h"

class QHostAddress;
class QTcpSocket;
class TrackHnd;

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
        wtAuth,
        wtLogBookBeg,
        wtLogBook,
        wtTrkListBeg,
        wtTrkList,
        wtTrackBeg,
        wtTrack
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
    bool requestLogBook(uint32_t beg = 50, uint32_t count = 50);
    bool requestTrackList();
    bool requestTrack(quint32 i);
    bool requestTrack(const trklist_item_t &trk);

    const QList<logbook_item_t> & logbook() const { return m_logbook; }
    const logbook_item_t &logbook(quint32 i) const;
    const QList<trklist_item_t> & trklist() const { return m_trklist; }
    const trklist_item_t &trklist(quint32 i) const;
    const TrackHnd *track() { return m_track; };

signals:
    void waitChange(NetProcess::wait_t);
    void rcvInit();
    void rcvAuth(bool isok);
    void rcvData(quint32 pos, quint32 max);
    void rcvLogBook();
    void rcvTrkList();
    void rcvTrack(const trkinfo_t &);

private:
    err_t m_err;
    wait_t m_wait;
    BinProto m_pro;
    NetTcpSocket m_nettcp;
    QTcpSocket *tcpClient;
    quint32 m_rcvpos, m_rcvcnt;
    QList<logbook_item_t> m_logbook;
    QList<trklist_item_t> m_trklist;
    TrackHnd *m_track;

    void tcpConnected();
    void tcpDisconnected();
    void tcpError();

    void setWait(wait_t _wait);
    void rcvProcess();
    void rcvWrong();
};

#endif // NETPROCESS_H
