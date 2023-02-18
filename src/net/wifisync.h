/*
    WiFi functions
*/

#ifndef _net_wifisync_H
#define _net_wifisync_H

#include "../../def.h"
#include <stdint.h>
#include <stddef.h>

namespace wSync { 
    typedef enum {
        stNotValid,
        stWiFiInit,
        stWiFiConnect,
        stSrvConnect,
        stSrvAuth,
        stProfileJoin,
        stSendConfig,
        stSendJumpCount,
        stSendPoint,
        stSendLogBook,
        stSendTrackList,
        stSendTrack,
        stSendDataFin,
        stRecvWiFiPass,
        stRecvVerAvail,
        stRecvFirmware,
        stFinOk,
        stUserCancel,
        errWiFiInit,
        errWiFiConnect,
        errTimeout,
        errSrvConnect,
        errSrvDisconnect,
        errRecvData,
        errRcvCmdUnknown,
        errSendData,
        errJoinLoad,
        errJoinSave,
        errWorker
    } st_t;
    
    typedef struct {
        uint32_t joinnum;
        uint16_t timeout;
        uint32_t cmplval, cmplsz;
    } info_t;
}

void wifiSyncBegin(const char *ssid, const char *pass = NULL);
wSync::st_t wifiSyncState(wSync::info_t &inf);
bool wifiSyncIsRun();
bool wifiSyncStop();
void wifiSyncDelete();

#endif // _net_wifisync_H
