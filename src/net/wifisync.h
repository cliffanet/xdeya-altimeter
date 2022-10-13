/*
    WiFi functions
*/

#ifndef _net_wifisync_H
#define _net_wifisync_H

#include "../../def.h"
#include <cstddef>

/*
class NetSocket;
class BinProtoSend;
class BinProtoRecv;

class WorkerWiFiSync : public WorkerProc
{
    public:
        enum {
            opExit,
            opClose,
            opOff,
            opProfileJoin,
            opWiFiConnect,
            opSrvConnect,
            opSrvAuth,
            opWaitAuth,
            opSndConfig,
            opSndJumpCount,
            opSndPoint,
            opSndLogBook,
            opSndTrack,
            opSndDataFin
        };
        
        typedef enum {
            stRun,
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
    
        WorkerWiFiSync(const char *ssid, const char *pass = NULL);
        ~WorkerWiFiSync();
        
        st_t st() const { return m_st; }
        bool isrun() const { return op() > opOff; }
        uint32_t joinnum() const { return d.rejoin_num; }
        
        void stop();
        void end();
        state_t process();

    private:
        st_t m_st;
        NetSocket *m_sock;
        BinProtoRecv *m_pro;
        WorkerProc::key_t m_wrk;
        
        union {
            uint32_t rejoin_num;
            struct __attribute__((__packed__)) {
                uint32_t authid;
                uint32_t secnum;
            } join;
            struct __attribute__((__packed__)) {
                uint32_t ckscfg;
                uint32_t cksjmp;
                uint32_t ckspnt;
                uint32_t ckslog;
                uint32_t poslog;
                FileTrack::chs_t ckstrack;
            } acc;
            
        } d;
        
        void initpro();
        void err(st_t st);
        bool recvdata(uint8_t cmd);
};

WorkerWiFiSync * wifiSyncProc();
*/

namespace wSync { 
    typedef enum {
        stNotRun,
        stWiFiInit,
        stWiFiConnect,
        stWiFiWait,
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
}

void wifiSyncBegin(const char *ssid, const char *pass = NULL);
wSync::st_t wifiSyncState();
bool wifiSyncStop();

#endif // _net_wifisync_H
