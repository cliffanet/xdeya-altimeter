/*
    WiFi functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "wifi.h"
#include "binproto.h"
#include "../view/text.h"


#define NEXT(op,tmr)        next(st ## op, PSTR(TXT_WIFI_MSG_ ## op), tmr)
#define ERR(s)              stop(PSTR(TXT_WIFI_ERR_ ## s))
#define FIN(s)              stop(PSTR(TXT_WIFI_MSG_ ## s))

#define RETURN_NEXT(op,tmr)     { NEXT(op,tmr); return STATE_RUN; }
#define RETURN_ERR(s)           { ERR(s); return STATE_WAIT; }
#define RETURN_FIN(s)           { FIN(s); return STATE_WAIT; }

WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) {
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

void WorkerWiFiSync::next(op_t op, const char *msg_P, uint32_t tmr) {
    m_msg_P = msg_P;
    m_op = op;
    settimer(tmr);
}

void WorkerWiFiSync::stop(const char *msg_P) {
    m_msg_P = msg_P;
    m_op = stOff;
}

void WorkerWiFiSync::cancel() {
    if (m_op > stCloseMsg)
        FIN(USERCANCEL);
}

WorkerWiFiSync::state_t
WorkerWiFiSync::process() {
    if (istimeout()) {
        ERR(TIMEOUT);
        clrtimer();
    }
    
    switch (m_op) {
        case stOff:
            wifiCli()->disconnect();
            wifiStop();
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
            if (!wifiCli()->connect())
                RETURN_ERR(SERVERCONNECT);
            
            RETURN_NEXT(SRVAUTH, 100);
        
        case stSRVAUTH:
        // авторизируемся на сервере
            //authStart();
            //return;
            return STATE_WAIT;
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
