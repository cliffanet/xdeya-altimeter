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

typedef struct {
    uint32_t val, sz;
} cmpl_t;

bool sendCfgMain(BinProto *pro);
bool sendJmpCount(BinProto *pro);
bool sendPoint(BinProto *pro);
WrkProc::key_t sendLogBook(BinProto *pro, uint32_t cks, int32_t pos, bool noremove = false);
bool isokLogBook(const WrkProc *_wrk = NULL);
bool sendDataFin(BinProto *pro);
WrkProc::key_t sendTrackList(BinProto *pro, bool noremove = false);
bool isokTrackList(const WrkProc *_wrk = NULL);

typedef struct __attribute__((__packed__)) {
    uint32_t id;
    uint32_t jmpnum;
    uint32_t jmpkey;
    tm_t     tmbeg;
    uint8_t  fnum;
} trksrch_t;
WrkProc::key_t sendTrack(BinProto *pro, const trksrch_t &srch, bool noremove = false);
bool isokTrack(const WrkProc *_wrk = NULL);
cmpl_t cmplTrack(const WrkProc *_wrk = NULL);

WrkProc::key_t recvWiFiPass(BinProto *pro, bool noremove = false);
bool isokWiFiPass(const WrkProc *_wrk = NULL);
WrkProc::key_t recvVerAvail(BinProto *pro, bool noremove = false);
bool isokVerAvail(const WrkProc *_wrk = NULL);
WrkProc::key_t recvFirmware(BinProto *pro, bool noremove = false);
bool isokFirmware(const WrkProc *_wrk = NULL);
cmpl_t cmplFirmware(const WrkProc *_wrk = NULL);

#endif // _net_sync_H
