#ifndef NETPROCESS_H
#define NETPROCESS_H

#include <QObject>
#include "BinProto/nettcpsocket.h"
#include "BinProto/BinProto.h"
#include "nettypes.h"
#include "trackhnd.h"

class QHostAddress;
class QTcpSocket;
class WiFiInfo;
class JmpInfo;
class TrkInfo;
class TrackHnd;

class NetProcess : public QObject
{
    Q_OBJECT
public:
    typedef enum {
        errNoError,
        errSock,
        errProto,
        errAuth
    } err_t;

    typedef enum {
        wtDisconnected = -1,
        wtUnknown = 0,
        wtConnecting,
        wtInit,
        wtAuthReq,
        wtAuth,
        wtWiFiBeg,
        wtWiFi,
        wtWiFiSend,
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
    quint32 rcvMax() const { return m_rcvcnt; }
    quint32 rcvPos() const { return m_rcvpos; }

    bool requestInit();
    bool requestAuth(uint16_t code);
    bool requestWiFiPass();
    bool sendWiFiPass();
    bool requestLogBook(uint32_t beg = 50, uint32_t count = 50);
    bool requestTrackList();
    bool requestTrack(int i);
    bool requestTrack(const trklist_item_t &trk);

    QList<WiFiInfo *> & wifipass() { return m_wifipass; } // это можно редактировать снаружи
    const QList<JmpInfo *> & logbook() const { return m_logbook; }
    const QList<TrkInfo *> & trklist() const { return m_trklist; }
    const TrackHnd *track() { return m_track; };

signals:
    void waitChange(NetProcess::wait_t);
    void rcvAuthReq();
    void rcvAuth(bool isok);
    void rcvCmdConfirm(uint8_t cmd, uint8_t err);
    void rcvData(quint32 pos, quint32 max);
    void rcvWiFiPass();
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
    QList<WiFiInfo *> m_wifipass;
    QList<JmpInfo *> m_logbook;
    QList<TrkInfo *> m_trklist;
    TrackHnd *m_track;

    void tcpConnected();
    void tcpDisconnected();
    void tcpError();

    void setWait(wait_t _wait);
    void rcvProcess();
    void rcvWrong();
};

#endif // NETPROCESS_H
