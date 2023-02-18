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

        Wrk2Net(BinProto *_pro, uint16_t _id = 0) :
            m_id(_id),
            m_pro(_pro),
            m_cmpl({ 0, 0 })
            {}

        uint16_t id() const { return m_id; };
        bool isnetok() const { return m_pro != NULL; }
        const cmpl_t& cmpl() const { return m_cmpl; };

    protected:
        uint16_t m_id = 0;
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

Wrk2Proc<Wrk2Net> recvWiFiPass(BinProto *pro);
Wrk2Proc<Wrk2Net> recvVerAvail(BinProto *pro);
Wrk2Proc<Wrk2Net> recvFirmware(BinProto *pro);

void netApp(NetSocket *sock);

#endif // _net_sync_H
