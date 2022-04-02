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

#endif // _net_sync_H
