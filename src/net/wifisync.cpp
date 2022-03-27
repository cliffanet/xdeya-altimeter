/*
    WiFi functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "wifi.h"
#include "binproto.h"
#include "../view/text.h"
#include "../cfg/webjoin.h"


#define NEXT(op,tmr)        next(st ## op, PSTR(TXT_WIFI_MSG_ ## op), tmr)
#define ERR(s)              stop(PSTR(TXT_WIFI_ERR_ ## s))
#define FIN(s)              stop(PSTR(TXT_WIFI_MSG_ ## s))

#define RETURN_NEXT(op,tmr)     { NEXT(op,tmr); return STATE_RUN; }
#define RETURN_ERR(s)           { ERR(s); return STATE_WAIT; }
#define RETURN_FIN(s)           { FIN(s); return STATE_WAIT; }

// отдельный retnext для recvdata(), который возвращает не state, а bool
#define RET_NEXT(op,tmr)        { NEXT(op,tmr); return true; }

WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_sock(NULL),
    m_proo(NULL)
{
    CONSOLE("[%08x] create", this);
    if (!wifiStart()) {
        ERR(WIFIINIT);
        return;
    }
    if (!wifiConnect(ssid, pass)) {
        ERR(WIFICONNECT);
        return;
    }
    
    NEXT(WIFICONNECT, 300);
}
WorkerWiFiSync::~WorkerWiFiSync() {
    CONSOLE("[%08x] destroy", this);
    if (m_proo != NULL)
        delete m_proo;
    if (m_proi != NULL)
        delete m_proi;
    if (m_sock != NULL)
        delete m_sock;
}

void WorkerWiFiSync::next(op_t op, const char *msg_P, uint32_t tmr) {
    m_msg_P = msg_P;
    m_op = op;
    settimer(tmr);
}

void WorkerWiFiSync::stop(const char *msg_P) {
    m_msg_P = msg_P;
    m_op = stOff;
    clrtimer();
}

void WorkerWiFiSync::cancel() {
    if (m_op > stCloseMsg)
        FIN(USERCANCEL);
}

void WorkerWiFiSync::initpro() {
    if ((m_sock == NULL) || (m_proo != NULL))
        return;
    
    m_proo = new BinProtoSend(m_sock, '%');
    // auth
    m_proo->add( 0x01, PSTR("N") );
    
    m_proi = new BinProtoRecv(m_sock, '#');
    // rejoin
    m_proi->add( 0x10, PSTR("N") );
    // accept
    m_proi->add( 0x20, PSTR("XXXXNH") );
}

void WorkerWiFiSync::end() {
    if (m_proo != NULL) {
        CONSOLE("delete m_proo");
        delete m_proo;
        m_proo = NULL;
    }
    if (m_proi != NULL) {
        CONSOLE("delete m_proi");
        delete m_proi;
        m_proi = NULL;
    }
    if (m_sock != NULL) {
        m_sock->disconnect();
        CONSOLE("delete m_sock");
        delete m_sock;
        m_sock = NULL;
    }
    wifiStop();
}

WorkerWiFiSync::state_t
WorkerWiFiSync::process() {
    if (istimeout()) {
        ERR(TIMEOUT);
        clrtimer();
    }
    
    if ((m_proi != NULL) && (m_op > stCloseMsg))
        switch (m_proi->process()) {
            case BinProtoRecv::STATE_OK:
                settimer(100);
                return STATE_RUN;
            
            case BinProtoRecv::STATE_CMD:
                {
                    uint8_t cmd;
                    if (!m_proi->recv(cmd, d))
                        RETURN_ERR(RCVDATA);
                    if (!recvdata(cmd))
                        RETURN_ERR(RCVCMDUNKNOWN);
                }
                return STATE_RUN;
                
            case BinProtoRecv::STATE_ERROR:
                RETURN_ERR(RCVDATA);
        }
    
    switch (m_op) {
        case stOff:
            end();
            if (m_msg_P != NULL) {
                m_op = stCloseMsg;
                return STATE_WAIT;
            }
            return STATE_END;
            
        case stWIFICONNECT:
        // ожидаем соединения по вифи
            switch (wifiStatus()) {
                case WIFI_STA_CONNECTED:
                    CONSOLE("wifi ok, try to server connect");
                    // вифи подключилось, соединяемся с сервером
                    RETURN_NEXT(SRVCONNECT, 100);
                
                case WIFI_STA_FAIL:
                    RETURN_ERR(WIFICONNECT);
            }
            
            return STATE_WAIT;
        
        case stSRVCONNECT:
        // ожидаем соединения к серверу
            if (m_sock == NULL)
                m_sock = wifiCliCreate();
            if (!m_sock->connect())
                RETURN_ERR(SERVERCONNECT);
            
            initpro();
            
            RETURN_NEXT(SRVAUTH, 100);
        
        case stSRVAUTH:
        // авторизируемся на сервере
            {
                ConfigWebJoin wjoin;
                if (!wjoin.load())
                    RETURN_ERR(JOINLOAD);

                CONSOLE("[authStart] authid: %lu", wjoin.authid());
            
                if (!m_proo->send(0x01, wjoin.authid()))
                    RETURN_ERR(SENDAUTH);
            }
            
            RETURN_NEXT(WAITAUTH, 200);
    }
    
    return STATE_WAIT;
}

bool WorkerWiFiSync::recvdata(uint8_t cmd) {
    switch (m_op) {
    
        case stWAITAUTH:
            switch (cmd) {
                case 0x10:
                    return true;
                case 0x20:
                    CONSOLE("auth: %08x %08x %08x %08x", d.acc.ckscfg, d.acc.cksjmp, d.acc.ckspnt, d.acc.ckslog);
                    RET_NEXT(SENDCONFIG, 10);
            }
            return false;
    }
    
    return false;
}


/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void wifiSyncBegin(const char *ssid, const char *pass) {
    if (wrkExists(WORKER_WIFI_SYNC))
        return;
    wrkAdd(WORKER_WIFI_SYNC, new WorkerWiFiSync(ssid, pass));
}

WorkerWiFiSync * wifiSyncProc() {
    return reinterpret_cast<WorkerWiFiSync *>(wrkGet(WORKER_WIFI_SYNC));
}
