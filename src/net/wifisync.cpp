/*
    WiFi functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "wifi.h"
#include "binproto.h"
#include "../view/text.h"


#define MSG(s)              m_msg_P = PSTR(TXT_WIFI_MSG_ ## s)
#define NEXT(st,s) { \
        m_op = st; \
        MSG(s); \
        return STATE_RUN; \
    }
#define ERR(s)              { stop(PSTR(TXT_WIFI_ERR_ ## s)); return STATE_WAIT; }
#define FIN(s)              stop(PSTR(TXT_WIFI_MSG_ ## s))

WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_msg_P(PSTR(TXT_WIFI_MSG_WIFICONNECT)),
    m_op(stWiFi)
{
    if (!wifiStart()) {
        stop(PSTR(TXT_WIFI_ERR_WIFIINIT));
        return;
    }
    if (!wifiConnect(ssid, pass)) {
        stop(PSTR(TXT_WIFI_ERR_WIFICONNECT));
        return;
    }
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
    switch (m_op) {
        case stOff:
            wifiCli()->disconnect();
            wifiStop();
            if (m_msg_P != NULL) {
                m_op = stCloseMsg;
                return STATE_WAIT;
            }
            return STATE_END;
            
        case stWiFi:
        // этап 1 - ожидаем соединения по вифи
            switch (wifiStatus()) {
                case WIFI_STA_CONNECTED:
                    CONSOLE("wifi ok, try to server connect");
                    // вифи подключилось, соединяемся с сервером
                    NEXT(stSrvConnect, SRVCONNECT);
                
                    // авторизаемся на сервере
                    //authStart();
                    //return;
                
                case WIFI_STA_FAIL:
                    ERR(WIFICONNECT);
            }
            
            //if (timeout == 1)
            //    ERR(WIFITIMEOUT);
            return STATE_WAIT;
        
        case stSrvConnect:
            if (!wifiCli()->connect())
                ERR(SERVERCONNECT);
            
            NEXT(stSrvAuth, SRVAUTH);
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
