/*
    WiFi for application functions
*/

#include "wifiapp.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "../core/worker.h"
#include "wifi.h"
#include "netsync.h"
#include "netsocket.h"
#include "../view/menu.h" // MENUSZ_VAL

#include <string.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <WiFiClient.h>

#include "esp_system.h" // esp_random

#define ERR(s)                  END

class _wifiApp : public Wrk {
        bool        m_cancel, m_wifi;
        uint16_t    m_timeout;
        const char  *m_ssid, *m_pass;
        int         m_bcast, m_srvsock;
        uint16_t    m_srvport;
    public:
        _wifiApp(const char *ssid, const char *pass) :
            m_cancel(false),
            m_wifi(false),
            m_timeout(0),
            m_bcast(0),
            m_srvsock(0),
            m_srvport(0)
        {
            // Приходится копировать, т.к. к моменту,
            // когда мы этими строками воспользуемся, их источник будет удалён.
            // Пробовал в конструкторе делать wifi connect,
            // но эти процессы не быстрые, лучше их оставить воркеру.
            m_ssid = strdup(ssid);
            m_pass = pass != NULL ? strdup(pass) : NULL;
            CONSOLE("begin: %s", m_ssid);
            //optset(O_NOREMOVE); потребуется, если будем использовать код ошибки выполнения
            
            // Для m_srvsock можно было бы использовать WiFiServer,
            // однако, нам бы знать, на каком порту он слушает, т.к. порт мы выбираем
            // случайным образом. А текущая реализация WiFiServer не позволяет это выяснить.
            // Чтобы не заниматься дублированием переменных, перепишем себе код реализации,
            // тем более, там просто, а WiFiServer перезаморочен на варианты применения
            // available/accept/hasClient
        }

        const char *ssid() const { return m_ssid; }
        
        /*
        state_t err(st_t st) {
            m_st = st;
            CONSOLE("err: %d", st);
            return END;
        }
        state_t chktimeout() {
            if (m_timeout == 0)
                return DLY;
            
            m_timeout--;
            if (m_timeout > 0)
                return DLY;
            
            return ERR(Timeout);
        }
        */
        
        void cancel() {
            if (!m_cancel) {
                //m_st = stUserCancel;
                m_cancel = true;
                m_timeout = 0;
            }
        }
    
    state_t run() {
    WPROC
        if (!wifiStart())
            return ERR(WiFiInit);
    
    WPRC_RUN
        CONSOLE("wifi to: %s; pass: %s", m_ssid, m_pass == NULL ? "-no-" : m_pass);
        if (!wifiConnect(m_ssid, m_pass))
            return ERR(WiFiConnect);
        m_timeout = 300;
    
    WPRC_RUN
        // отмена работы по wifi
        if (m_cancel)
            return END;
        // ожидаем соединения по вифи и проверяем состояние
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
            if (m_bcast>0) {
                close(m_bcast);
                CONSOLE("bcast socket closed (fd: %d)", m_bcast);
                m_bcast = 0;
            }
            if (m_srvsock>0) {
                close(m_srvsock);
                CONSOLE("server socket closed (fd: %d; port: %d)", m_srvsock, m_srvport);
                m_srvsock = 0;
                m_srvport = 0;
            }
            
            if (m_timeout > 0)
                m_timeout--;
            if (m_timeout == 0) {
                CONSOLE("wifi connect timeout: %d", wst);
                if (!wifiConnect(m_ssid, m_pass))
                    return ERR(WiFiConnect);
                m_timeout = 300;
            }
            
            return DLY;
        }
        
        // Широковещательный сокет для DeviceDiscovery
        if (m_bcast == 0) {
            m_bcast = socket(AF_INET, SOCK_DGRAM, 0);
            int opt=1;
            if (setsockopt(m_bcast, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0)
                return ERR(SockOpt);
            CONSOLE("bcast created (fd: %d)", m_bcast);
        }
        
        if (m_timeout > 0)
            m_timeout--;
        if ((m_srvsock > 0) && (m_timeout == 0)) {
            struct sockaddr_in s;
            bzero(&s, sizeof(s));
            s.sin_family = AF_INET;
            s.sin_port = (in_port_t)htons(3310);
            s.sin_addr.s_addr = htonl(INADDR_BROADCAST);
            
            char mess[] = "\0\0XdeYa";
            uint16_t port = htons(m_srvport);
            memcpy(mess, &port, sizeof(port));
            if(sendto(m_bcast, mess, sizeof(mess)-1, 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
                return ERR(SockSend);
            CONSOLE("send to bcast (port: %d)", m_srvport);
            m_timeout = 45;
        }
        
        // Сервер
        if (m_srvsock == 0) {
            m_srvsock = socket(AF_INET, SOCK_STREAM, 0);
            CONSOLE("server socket created (fd: %d)", m_srvsock);
            int reuse = 1;
            setsockopt(m_srvsock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            
            m_srvport = 35005 + (esp_random() % 256);
            struct sockaddr_in srv;
            srv.sin_family = AF_INET;
            srv.sin_addr.s_addr = htonl(INADDR_ANY);
            srv.sin_port = htons(m_srvport);
            if (bind(m_srvsock, (struct sockaddr *)&srv, sizeof(srv)) < 0)
                return ERR(SockSrvBind);
            if (listen(m_srvsock, 4) < 0)
                return ERR(SockSrvListen);
            fcntl(m_srvsock, F_SETFL, O_NONBLOCK);
            CONSOLE("server socket listen (port: %d)", m_srvport);
        }
        
        if (m_srvsock > 0) {
            struct sockaddr_in addr;
            int len = sizeof(struct sockaddr_in);
#ifdef ESP_IDF_VERSION_MAJOR
            int sock = lwip_accept(m_srvsock, (struct sockaddr *)&addr, (socklen_t*)&len);
#else
            int sock = lwip_accept_r(m_srvsock, (struct sockaddr *)&addr, (socklen_t*)&len);
#endif
            if (sock > 0) {
                uint8_t ip[4];
                memcpy(ip, &addr.sin_addr, sizeof(ip));
                CONSOLE("request connection from: %d.%d.%d.%d : %d", ip[0], ip[1], ip[2], ip[3], ntohs(addr.sin_port));
                netApp(new NetSocketClient<WiFiClient>(WiFiClient(sock)));
            }
        }
        
    WPRC(DLY);
    }
        
    void end() {
        if (m_bcast>0) {
            close(m_bcast);
            CONSOLE("bcast socket closed (fd: %d)", m_bcast);
            m_bcast = 0;
        }
        if (m_srvsock>0) {
            close(m_srvsock);
            CONSOLE("server socket closed (fd: %d; port: %d)", m_srvsock, m_srvport);
            m_srvsock = 0;
            m_srvport = 0;
        }
        
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
static WrkProc<_wifiApp> _app;
void wifiCliBegin(const char *ssid, const char *pass) {
    if (!_app.isrun())
        _app = wrkRun<_wifiApp>(ssid, pass);
}

bool wifiCliNet(char *ssid) {
    if (_app.isrun()) {
        strncpy(ssid, _app->ssid(), MENUSZ_VAL);
        ssid[MENUSZ_VAL-1] = '\0';
    }
    else
        *ssid = '\0';
    
    return _app.isrun();
}

bool wifiCliIsRun() {
    return _app.isrun();
}
