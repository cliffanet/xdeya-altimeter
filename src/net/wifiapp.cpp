/*
    WiFi for application functions
*/

#include "wifiapp.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "../core/worker.h"
#include "wifi.h"
#include "../view/menu.h" // MENUSZ_VAL

#include <string.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>


//#define ERR(s)                  err(err ## s)
#define ERR(s)
//#define RETURN_ERR(s)           return ERR(s)
#define RETURN_ERR(s)           WRK_RETURN_FINISH 

WRK_DEFINE(WIFI_CLI) {
    public:
        bool        m_cancel, m_wifi;
        uint16_t    m_timeout;
        const char  *m_ssid, *m_pass;
        int         m_bcast;
        /*
        NetSocket   *m_sock;
        BinProto    m_pro;
        WrkProc::key_t m_wrk;
        */
        
        WRK_CLASS(WIFI_CLI)(const char *ssid, const char *pass) :
            m_cancel(false),
            m_wifi(false),
            m_timeout(0),
            m_bcast(0)
            /*
            m_sock(NULL),
            m_pro(NULL, '%', '#'),
            m_wrk(WRKKEY_NONE),
            */
        {
            // Приходится копировать, т.к. к моменту,
            // когда мы этими строками воспользуемся, их источник будет удалён.
            // Пробовал в конструкторе делать wifi connect,
            // но эти процессы не быстрые, лучше их оставить воркеру.
            m_ssid = strdup(ssid);
            m_pass = pass != NULL ? strdup(pass) : NULL;
            //optset(O_NOREMOVE); не требуется
        }
        
        /*
        state_t err(st_t st) {
            m_st = st;
            CONSOLE("err: %d", st);
            WRK_RETURN_FINISH;
        }
        */
        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            RETURN_ERR(Timeout);
        }
        
        void cancel() {
            if (isrun()) {
                //m_st = stUserCancel;
                m_cancel = true;
                m_timeout = 0;
            }
        }
    
    // Это выполняем всегда перед входом в process
    state_t every() {
        /*
        if (m_wrk != WRKKEY_NONE) {
            // Ожидание выполнения дочернего процесса
            auto wrk = _wrkGet(m_wrk);
            if (wrk == NULL) {
                CONSOLE("worker[key=%d] finished unexpectedly", m_wrk);
                RETURN_ERR(Worker);
            }
            if (wrk->isrun())
                WRK_RETURN_WAIT;
            
            CONSOLE("worker[key=%d] finished", m_wrk);
            
            // Удаляем завершённый процесс
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
        }
        */
        
        if (m_cancel)
            WRK_RETURN_FINISH;
        
        /*
        if ((m_sock != NULL) && m_sock->connected()) {
            m_pro.rcvprocess();
            if (!m_pro.rcvvalid())
                RETURN_ERR(RecvData);
        }
        */
        
        WRK_RETURN_RUN;
    }
    
    WRK_PROCESS
        if (!wifiStart())
            RETURN_ERR(WiFiInit);
    
    WRK_BREAK_RUN
        CONSOLE("wifi to: %s; pass: %s", m_ssid, m_pass == NULL ? "-no-" : m_pass);
        if (!wifiConnect(m_ssid, m_pass))
            RETURN_ERR(WiFiConnect);
        m_timeout = 300;
    
    WRK_BREAK_RUN
        // ожидаем соединения по вифи
        auto wst = wifiStatus();
        if (wst == WIFI_STA_CONNECTED) {
            if (!m_wifi) {
                m_wifi = true;
                CONSOLE("wifi ok");
                m_timeout = 0;
            }
        }
        else {
            if (m_wifi) {
                m_wifi = false;
                CONSOLE("wifi fail: %d", wst);
                m_timeout = 300;
            }
            if (m_timeout > 0)
                m_timeout--;
            if (m_timeout == 0) {
                CONSOLE("wifi connect timeout: %d", wst);
                if (!wifiConnect(m_ssid, m_pass))
                    RETURN_ERR(WiFiConnect);
                m_timeout = 300;
            }
            WRK_RETURN_WAIT;
        }
        
        if (m_bcast == 0) {
            m_bcast = socket(AF_INET, SOCK_DGRAM, 0);
            int opt=1;
            if (setsockopt(m_bcast, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
                RETURN_ERR(SockOpt);
            CONSOLE("bcast created");
        }
        
        if (m_timeout > 0)
            m_timeout--;
        if (m_timeout == 0) {
            struct sockaddr_in s;
            bzero(&s, sizeof(s));
            s.sin_family = AF_INET;
            s.sin_port = (in_port_t)htons(3310);
            s.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            
            const char mess[] = "\0\0XdeYa";
            if(sendto(m_bcast, mess, sizeof(mess)-1, 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
                RETURN_ERR(SockSend);
            CONSOLE("send to bcast");
            m_timeout = 50;
        }
        
        WRK_RETURN_WAIT;
    WRK_FINISH
        
    void end() {
        /*
        if (m_wrk != WRKKEY_NONE) {
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
        }
        
        m_pro.sock_clear();
        
        if (m_sock != NULL) {
            m_sock->disconnect();
            CONSOLE("delete m_sock");
            delete m_sock;
            m_sock = NULL;
        }
        */
        
        wifiStop();
        
        if (m_ssid != NULL) {
            delete m_ssid;
            m_ssid = NULL;
        }
        if (m_pass != NULL) {
            delete m_pass;
            m_pass = NULL;
        }
    }
};

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void wifiCliBegin(const char *ssid, const char *pass) {
    if (wrkExists(WIFI_CLI))
        return;
    wrkRun(WIFI_CLI, ssid, pass);
    CONSOLE("begin");
}

void wifiCliNet(char *ssid) {
    auto wrk = wrkGet(WIFI_CLI);  
    if ((wrk == NULL) || !wrk->isrun() || (wrk->m_ssid == NULL))
        *ssid = '\0';
    else {
        strncpy(ssid, wrk->m_ssid, MENUSZ_VAL);
        ssid[MENUSZ_VAL-1] = '\0';
    }
}
