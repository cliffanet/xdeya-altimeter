/*
    Worker add local info
*/

#ifndef _core_worker_local_H
#define _core_worker_local_H

#include "worker.h"

enum {
    WRKKEY_NONE = 0,
    
    WRKKEY_NAVI_INIT,
    WRKKEY_TRK_SAVE,
    WRKKEY_WIFI_SYNC,
    WRKKEY_SEND_LOGBOOK,
    WRKKEY_SEND_TRACKLIST,
    WRKKEY_SEND_TRACK,
    WRKKEY_RECV_WIFIPASS,
    WRKKEY_RECV_VERAVAIL,
    WRKKEY_RECV_FIRMWARE,
    
    WRKKEY_RAND_MIN,
    WRKKEY_RAND_MAX = WRKKEY_RAND_MIN + 20
};

#endif // _core_worker_local_H
