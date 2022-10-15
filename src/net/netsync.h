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
bool sendLogBook(BinProto *pro, uint32_t _cks, uint32_t _pos);
bool sendDataFin(BinProto *pro);

#endif // _net_sync_H
