/*
    WiFi functions
*/

#ifndef _net_wifisync_H
#define _net_wifisync_H

#include "../../def.h"
#include <cstddef>
#include "../core/worker.h"

class NetSocket;
class BinProtoSend;
class BinProtoRecv;

class WorkerWiFiSync : public WorkerProc
{
    public:
        typedef enum {
            stOff,
            stCloseMsg,
            stWIFICONNECT,
            stSRVCONNECT,
            stSRVAUTH,
            stWAITAUTH,
            stSENDCONFIG
        } op_t;
    
        WorkerWiFiSync(const char *ssid, const char *pass = NULL);
        ~WorkerWiFiSync();
        
        op_t op() const { return m_op; }
        const char *msg_P() const { return m_msg_P; }

        void stop(const char *msg_P = NULL);
        void cancel();
        void end();
        state_t process();

    private:
        op_t m_op;
        const char *m_msg_P;
        NetSocket *m_sock;
        BinProtoSend *m_proo;
        BinProtoRecv *m_proi;
        
        union {
            uint32_t rejoin_num;
            struct __attribute__((__packed__)) {
                uint32_t ckscfg;
                uint32_t cksjmp;
                uint32_t ckspnt;
                uint32_t ckslog;
                uint32_t poslog;
                //FileTrack::chs_t ckstrack;
                uint64_t ckstrack;
            } acc;
        } d;
        
        void initpro();
        void next(op_t op, const char *msg_P, uint32_t tmr);
        bool recvdata(uint8_t cmd);
};

void wifiSyncBegin(const char *ssid, const char *pass = NULL);
WorkerWiFiSync * wifiSyncProc();

#endif // _net_wifisync_H
