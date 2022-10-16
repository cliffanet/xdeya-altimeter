/*
    Data transfere functions
*/

#ifndef _net_sync_H
#define _net_sync_H

#include "../../def.h"
#include "../core/worker.h"

class BinProto;

bool sendCfgMain(BinProto *pro);
bool sendJmpCount(BinProto *pro);
bool sendPoint(BinProto *pro);
WrkProc::key_t sendLogBook(BinProto *pro, uint32_t cks, int32_t pos, bool noremove = false);
bool isokLogBook(const WrkProc *_wrk = NULL);
bool sendDataFin(BinProto *pro);

WrkProc::key_t recvWiFiPass(BinProto *pro, bool noremove = false);
bool isokWiFiPass(const WrkProc *_wrk);

#endif // _net_sync_H
