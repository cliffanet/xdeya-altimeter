/*
    Data transfere functions
*/

#ifndef _net_sync_H
#define _net_sync_H

#include "../../def.h"
#include "../core/worker.h"
#include "../clock.h"
#include <stdint.h>
#include <stddef.h>

class BinProto;
class NetSocket;

typedef struct {
    uint32_t val, sz;
} cmpl_t;

typedef struct {
    uint32_t beg, count;
} posi_t;

enum {
    netUnknown,
    netSendLogBook,
    netSendWiFiPass,
    netSendTrackList,
    netSendTrack,
    netRecvWiFiPass,
    netRecvVerAvail,
    netRecvFirmware
};


class Wrk2Net : public Wrk2Ok {
    public:
        typedef struct {
            uint32_t val, sz;
        } cmpl_t;

        Wrk2Net(BinProto *pro) : m_pro(pro), m_cmpl({ 0, 0 }) {}
        bool isnetok() const { return m_pro != NULL; }
        const cmpl_t& cmpl() const { return m_cmpl; };

    protected:
        BinProto *m_pro;
        cmpl_t m_cmpl;
};

bool sendCfgMain(BinProto *pro);
bool sendJmpCount(BinProto *pro);
bool sendPoint(BinProto *pro);
Wrk2Proc<Wrk2Net> sendLogBook(BinProto *pro, uint32_t cks, uint32_t pos);
Wrk2Proc<Wrk2Net> sendLogBook(BinProto *pro, const posi_t &posi);
bool sendDataFin(BinProto *pro);
Wrk2Proc<Wrk2Net> sendWiFiPass(BinProto *pro);
Wrk2Proc<Wrk2Net> sendTrackList(BinProto *pro);

typedef struct __attribute__((__packed__)) {
    uint32_t id;
    uint32_t jmpnum;
    uint32_t jmpkey;
    tm_t     tmbeg;
    uint8_t  fnum;
} trksrch_t;
Wrk2Proc<Wrk2Net> sendTrack(BinProto *pro, const trksrch_t &srch);

WrkProc::key_t recvWiFiPass(BinProto *pro, bool noremove = false);
bool isokWiFiPass(const WrkProc *_wrk = NULL);
WrkProc::key_t recvVerAvail(BinProto *pro, bool noremove = false);
bool isokVerAvail(const WrkProc *_wrk = NULL);
WrkProc::key_t recvFirmware(BinProto *pro, bool noremove = false);
bool isokFirmware(const WrkProc *_wrk = NULL);
cmpl_t cmplFirmware(const WrkProc *_wrk = NULL);

void netApp(NetSocket *sock);

#endif // _net_sync_H
