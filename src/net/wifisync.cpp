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

WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_sock(NULL),
    bpo(NULL)
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
    if (m_sock != NULL)
        delete m_sock;
    if (bpo != NULL)
        delete bpo;
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

void WorkerWiFiSync::initbpo() {
    if ((m_sock == NULL) || (bpo != NULL))
        return;
    
    bpo = new BinProtoSend(m_sock, '%');
    // auth
    bpo->add( 0x01, PSTR("N") );
}

void WorkerWiFiSync::end() {
    if (m_sock != NULL) {
        m_sock->disconnect();
        CONSOLE("delete m_sock");
        delete m_sock;
        m_sock = NULL;
    }
    if (bpo != NULL) {
        CONSOLE("delete bpo");
        delete bpo;
        bpo = NULL;
    }
    wifiStop();
}

WorkerWiFiSync::state_t
WorkerWiFiSync::process() {
    if (istimeout()) {
        ERR(TIMEOUT);
        clrtimer();
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
            
            initbpo();
            
            RETURN_NEXT(SRVAUTH, 100);
        
        case stSRVAUTH:
        // авторизируемся на сервере
            {
                ConfigWebJoin wjoin;
                if (!wjoin.load())
                    RETURN_ERR(JOINLOAD);

                CONSOLE("[authStart] authid: %lu", wjoin.authid());
            
                if (!bpo->send(0x01, wjoin.authid()))
                    RETURN_ERR(SENDAUTH);
            }
            
            RETURN_NEXT(WAITAUTH, 200);
    }
    
    return STATE_WAIT;
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
