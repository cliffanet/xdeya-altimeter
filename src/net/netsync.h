/*
    Data transfere functions
*/

#ifndef _net_sync_H
#define _net_sync_H

#include "../../def.h"
#include "../core/worker.h"

class NetSocket;

bool sendCfgMain(NetSocket *nsock);
bool sendJmpCount(NetSocket *nsock);
bool sendPoint(NetSocket *nsock);
bool sendLogBook(NetSocket *nsock, uint32_t _cks, uint32_t _pos);
bool sendDataFin(NetSocket *nsock);

#endif // _net_sync_H
