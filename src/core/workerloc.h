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
    
    WRKKEY_RAND_MIN,
    WRKKEY_RAND_MAX = WRKKEY_RAND_MIN + 20
};

#endif // _core_worker_local_H
