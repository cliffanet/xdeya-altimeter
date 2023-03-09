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
    netSendFiles,
    netRecvWiFiPass,
    netRecvVerAvail,
    netRecvFirmware,
    netRecvFiles
};


class WrkNet : public WrkCmpl {
    public:
        WrkNet(BinProto *_pro, uint16_t _id = 0) :
            m_id(_id),
            m_pro(_pro)
            {}

        uint16_t id() const { return m_id; };
        bool isnetok() const { return m_pro != NULL; }

    protected:
        uint16_t m_id = 0;
        BinProto *m_pro;
};

bool sendCfgMain(BinProto *pro);
bool sendJmpCount(BinProto *pro);
bool sendPoint(BinProto *pro);
WrkProc<WrkNet> sendLogBook(BinProto *pro, uint32_t cks, uint32_t pos);
WrkProc<WrkNet> sendLogBook(BinProto *pro, const posi_t &posi);
bool sendDataFin(BinProto *pro);
WrkProc<WrkNet> sendWiFiPass(BinProto *pro);
WrkProc<WrkNet> sendTrackList(BinProto *pro);

typedef struct __attribute__((__packed__)) {
    uint32_t id;
    uint32_t jmpnum;
    uint32_t jmpkey;
    tm_t     tmbeg;
    uint8_t  fnum;
} trksrch_t;
WrkProc<WrkNet> sendTrack(BinProto *pro, const trksrch_t &srch);

WrkProc<WrkNet> recvWiFiPass(BinProto *pro);
WrkProc<WrkNet> recvVerAvail(BinProto *pro);
WrkProc<WrkNet> recvFirmware(BinProto *pro);

void netApp(NetSocket *sock);
uint16_t netAppCount();

#endif // _net_sync_H
