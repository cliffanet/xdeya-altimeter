/*
    Worker add local info
*/

#ifndef _core_worker_local_H
#define _core_worker_local_H

#include "worker.h"

enum {
    WORKER_NONE = 0,
    
    WORKER_NAV_INIT,
    WORKER_TRK_SAVE,
    WORKER_WIFI_SYNC,
    
    WORKER_RAND_MIN,
    WORKER_RAND_MAX = WORKER_RAND_MIN + 20
};

#endif // _core_worker_local_H
