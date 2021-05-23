
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../file/wifi.h"
#include "../file/veravail.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../net/srv.h"
#include "../net/data.h"
#include "../net/wifi.h"

#include <WiFi.h>
#include <Update.h>           // Обновление прошивки
#include <vector>

/* ------------------------------------------------------------------------------------------- *
 *  ViewNetSync - процесс синхронизации с сервером
 * ------------------------------------------------------------------------------------------- */
typedef enum {
    NS_NONE,
    NS_WIFI_CONNECT,
    NS_SERVER_HELLO,
    NS_PROFILE_JOIN,
    NS_SERVER_DATA_CONFIRM,
    NS_RCV_WIFI_PASS,
    NS_RCV_VER_AVAIL,
    NS_RCV_FWUPDATE,
    NS_EXIT
} netsync_state_t;

class ViewNetSync : public ViewBase {
    public:
        // старт процедуры синхронизации
        void begin(const char *_ssid, const char *_pass) {
            strcpy(ssid, _ssid);        // Приходится временно копировать ssid, т.к. оно находится внутри списка wifi для ViewMenuWifiSync
            if (_pass == NULL)          // а при завершении текущего begin этот список wifi будет очищен.
                pass[0] = '\0';
            else
                strcpy(pass, _pass);
    
            snprintf_P(title, sizeof(title), PSTR("wifi to %s"), ssid);
            setState(NS_WIFI_CONNECT, 10000);
            joinnum = 0;
    
            CONSOLE("wifi to: %s; pass: %s", ssid, pass);
            WiFi.mode(WIFI_STA);
            if (_pass == NULL)
                WiFi.begin((const char *)ssid);
            else
                WiFi.begin((const char *)ssid, pass);
        }
        
        void updTimeout(int32_t _timeout = 3000) {
            if (_timeout > 0)
                timeout = millis() + _timeout;
        }
        
        // изменение этапа
        void setState(netsync_state_t _state, int32_t _timeout = -1) {
            state = _state;
            updTimeout(_timeout);
        }
        
        // вывод сообщения
#define MSG(s) msg(PSTR(s))
        void msg(const char *_title) {
            if (_title == NULL)
                title[0] = '\0';
            else {
                strncpy_P(title, _title, sizeof(title));
                title[sizeof(title)-1] = '\0';
                CONSOLE("msg: %s", title);
            }
            displayUpdate();
        }
        
        // сообщение о переходе на следующий этап
#define NEXT(s, h, tout) next(PSTR(s), h, tout)
        void next(const char *_title, netsync_state_t _state, int32_t _timeout = -1) {
            msg(_title);
            setState(_state, _timeout);
        }
        
        // завершение всего процесса, но выход не сразу, а через несколько сек,
        // чтобы прочитать сообщение
#define ERR(s) fin(PSTR(s))
#define FIN(s) fin(PSTR(s))
        void fin(const char *_title) {
            srvStop();
            WiFi.mode(WIFI_OFF);
            next(_title, NS_EXIT, 5000);
        }
        
        // завершение всего процесса синхронизации с мнгновенным переходом в главный экран
        void close() {
            srvStop();
            WiFi.mode(WIFI_OFF);
            setViewMain();
        }
        
        // проверка, всё ли нормально с сервером
        bool chksrv() {
            const char *e = srvErr(); // ошибка это строка из progmem
            if (e != NULL) {
                fin(e);
                return false;
            }
            if (timeout < millis()) {
                ERR("timeout");
                return false;
            }
            return true;
        }
        
        // старт авторизации на сервере
        void authStart() {
            ConfigWebJoin wjoin;
            if (!wjoin.load()) {
                ERR("Can\'t load JOIN-inf");
                return;
            }

            CONSOLE("[authStart] authid: %lu", wjoin.authid());

            uint32_t id = htonl(wjoin.authid());
            if (!srvSend(0x01, id)) {
                ERR("auth send fail");
                return;
            }
            
            NEXT("wait hello", NS_SERVER_HELLO, 3000);
        }
        
        // пересылка данных на сервер
        void dataToServer(const daccept_t &acc) {
            CONSOLE("dataToServer");
    
            if ((acc.ckscfg == 0) || (acc.ckscfg != cfg.chksum())) {
                MSG("Sending config...");
                if (!sendCfg()) {
                    ERR("send cfg fail");
                    return;
                }
            }
    
            if ((acc.cksjmp == 0) || (acc.cksjmp != jmp.chksum())) {
                MSG("Sending jump count...");
                if (!sendJump()) {
                    ERR("send cmp count fail");
                    return;
                }
            }
    
            if ((acc.ckspnt == 0) || (acc.ckspnt != pnt.chksum())) {
                MSG("Sending point...");
                if (!sendPoint()) {
                    ERR("send pnt fail");
                    return;
                }
            }
    
            MSG("Sending LogBook...");
            if (!sendLogBook(acc.ckslog, acc.poslog)) {
                ERR("send LogBook fail");
                return;
            }
    
            MSG("Sending Tracks...");
            if (!sendTrack(acc.ckstrack)) {
                ERR("send Tracks fail");
                return;
            }
            
            MSG("Sending fin...");
            if (!sendDataFin()) {
                ERR("Finishing send fail");
                return;
            }
    
            NEXT("Wait server confirm...", NS_SERVER_DATA_CONFIRM, 30000);
        }
        
        // фоновый процессинг - обрабатываем этапы синхронизации
        void process() {
            switch (state) {
                case NS_WIFI_CONNECT:
                // этап 1 - ожидаем соединения по вифи
                    if (WiFi.status() == WL_CONNECTED) {
                        // вифи подключилось, соединяемся с сервером
                        if (!srvConnect()) {
                            ERR("server can't connect");
                            return;
                        }
                        
                        // авторизаемся на сервере
                        authStart();
                        return;
                    }
                    
                    // проблемы при соединении с вифи
                    if ((WiFi.status() == WL_NO_SSID_AVAIL) || (WiFi.status() == WL_CONNECT_FAILED)) {
                        ERR("wifi connect fail");
                        return;
                    }
    
                    if (timeout < millis())
                        ERR("wifi timeout");
                    return;
                
                case NS_SERVER_HELLO:
                // этап 2 - ожидаем реакцию на авторизацию с сервера
                    {
                        uint8_t cmd;
                        union {
                            uint32_t num;
                            daccept_t acc;
                        } d;
                        if (!srvRecv(cmd, d)) { // ожидание данных с сервера
                            chksrv();
                            return;
                        }
    
                        CONSOLE("[waitHello] cmd: %02x", cmd);
    
                        switch (cmd) {
                            case 0x10: // rejoin
                                // требуется привязка к аккаунту
                                joinnum = ntohl(d.num);
                                setState(NS_PROFILE_JOIN, 120000);
                                return;
        
                            case 0x20: // accept
                                // стартуем передачу данных на сервер
                                d.acc.ckscfg = ntohl(d.acc.ckscfg);
                                d.acc.cksjmp = ntohl(d.acc.cksjmp);
                                d.acc.ckspnt = ntohl(d.acc.ckspnt);
                                d.acc.ckslog = ntohl(d.acc.ckslog);
                                d.acc.poslog = ntohl(d.acc.poslog);
                                d.acc.ckstrack = ntocks(d.acc.ckstrack);
                                dataToServer(d.acc);
                                return;
                        }
    
                        ERR("recv unknown cmd");
                    }
                    return;
                
                case NS_PROFILE_JOIN:
                // этап 3 - устройство не прикреплено ни к какому профайлу, ожидается привязка
                    {
                        uint8_t cmd;
                        struct __attribute__((__packed__)) {
                            uint32_t authid;
                            uint32_t secnum;
                        } d;
                        if (!srvRecv(cmd, d)) {
                            if (!chksrv())
                                return;
                            
                            static uint8_t n = 0;
                            n++;
                            if (n > 16) {
                                // Отправляем idle- чтобы на сервере не сработал таймаут передачи
                                srvSend(0x12, htonl(timeout - millis()));
                                n=0;
                            }
                            return;
                        }
    
                        if (cmd != 0x13) {
                            ERR("recv unknown cmd");
                            return;
                        }
    
                        d.authid = ntohl(d.authid);
                        d.secnum = ntohl(d.secnum);
    
                        CONSOLE("[waitJoin] cmd: %02x (%lu, %lu)", cmd, d.authid, d.secnum);
    
                        ConfigWebJoin wjoin(d.authid, d.secnum);
                        if (!wjoin.save()) {
                            ERR("save fail");
                            return;
                        }
    
                        if (!srvSend(0x14)) {
                            ERR("join send fail");
                            return;
                        }
    
                        joinnum = 0;
                        MSG("join accepted");
    
                        authStart();
                    }
                    return;
                
                case NS_SERVER_DATA_CONFIRM:
                // этап 4 - после пересылки данных на сервер, ожидаем с сервера подтверждение об их приёме
                    {
                        uint8_t cmd;
                        if (!srvRecv(cmd)) {
                            chksrv();
                            return;
                        }
    
                        CONSOLE("[waitFin] cmd: %02x", cmd);
    
                        switch (cmd) {
                            case 0x41: // wifi beg
                                if (!wifiPassClear()) {
                                    ERR("WiFi clear Fail");
                                    return;
                                }
                                NEXT("Recv wifi pass", NS_RCV_WIFI_PASS, 3000);
                                return;
                                
                            case 0x44: // veravail beg
                                if (!verAvailClear()) {
                                    ERR("Versions clear Fail");
                                    return;
                                }
                                NEXT("Recv FW-versions", NS_RCV_VER_AVAIL, 3000);
                                return;
                                
                            case 0x47: // firmware update beg
                                NEXT("FW-update", NS_RCV_FWUPDATE, 3000);
                                return;
        
                            case 0x0f: // bye
                                FIN("Sync finished");
                                return;
                        }
    
                        ERR("recv unknown cmd");
                    }
                    return;
                
                case NS_RCV_WIFI_PASS:
                // этап 5 - принимаем список паролей для вифи-сетей
                    {
                        uint8_t cmd;
                        struct __attribute__((__packed__)) {
                            uint8_t s[512];
                        } d;
                        char ssid[33], pass[33];
                        uint8_t *snext = NULL;
                        uint32_t cks;

                        if (!chksrv())
                            return;
    
                        while (srvRecv(cmd, d)) {
                            switch (cmd) {
                                case 0x42: // wifi net
                                    ntostrs(ssid, sizeof(ssid), d.s, &snext);
                                    ntostrs(pass, sizeof(pass), snext);
                                    if (!wifiPassAdd(ssid, pass)) {
                                        ERR("WiFi add Fail");
                                        return;
                                    }
                                    updTimeout();
                                    break;
            
                                case 0x43: // wifi end
                                    cks = wifiPassChkSum();
                                    cks = htonl(cks);
                                    NEXT("Wait server fin...", NS_SERVER_DATA_CONFIRM, 3000);
                                    srvSend(0x4a, cks); // wifiok
                                    return;
            
                                default:
                                    ERR("recv unknown cmd");
                                    return;
                            }
                        }
                    }
                    return;
                
                case NS_RCV_VER_AVAIL:
                // этап 6 - принимаем список доступных версий прошивки
                    {
                        uint8_t cmd;
                        uint8_t s[512];
                        char ver[33];
                        uint32_t cks;

                        if (!chksrv())
                            return;
    
                        while (srvRecv(cmd, s, sizeof(s))) {
                            switch (cmd) {
                                case 0x45: // veravail item
                                    ntostrs(ver, sizeof(ver), s);
                                    if (!verAvailAdd(ver)) {
                                        ERR("FW-version add Fail");
                                        return;
                                    }
                                    updTimeout();
                                    break;
            
                                case 0x46: // veravail end
                                    cks = verAvailChkSum();
                                    cks = htonl(cks);
                                    NEXT("Wait server fin...", NS_SERVER_DATA_CONFIRM, 3000);
                                    srvSend(0x4b, cks); // veravail ok
                                    return;
            
                                default:
                                    ERR("recv unknown cmd");
                                    return;
                            }
                        }
                    }
                    return;
                
                case NS_RCV_FWUPDATE:
                // этап 7 - обновляем прошивку
                    {
                        uint8_t cmd;
                        union {
                            struct {
                                uint32_t    size;
                                char        md5[36];
                            } info;
                            struct {
                                uint16_t    sz;
                                uint8_t     buf[1000];
                            } data;
                        } d;

                        if (!chksrv())
                            return;
    
                        while (srvRecv(cmd, d)) {
                            switch (cmd) {
                                case 0x48: // fwupd info
                                    CONSOLE("recv fw info");
                                    {
                                        uint32_t freesz = ESP.getFreeSketchSpace();
                                        uint32_t cursz = ESP.getSketchSize();
                                        d.info.size = ntohl(d.info.size);
                                        d.info.md5[sizeof(d.info.md5)-1] = '\0';
                                        CONSOLE("current fw size: %lu, avail size for new fw: %lu, new fw size: %lu; md5: %s", 
                                                cursz, freesz, d.info.size, d.info.md5);
                                        
                                        if (d.info.size > freesz) {
                                            ERR("FW-size too big");
                                            return;
                                        }
            
                                        // start burn
                                        if (!Update.begin(d.info.size, U_FLASH) || !Update.setMD5(d.info.md5)) {
                                            CONSOLE("Upd begin fail: errno=%d", Update.getError());
                                            ERR("FW-init fail");
                                            return;
                                        }
                                    }
                                    break;
                                    
                                case 0x49: // fwupd data
                                    {
                                        d.data.sz = ntohs(d.data.sz);
                                        int sz1 = Update.write(d.data.buf, d.data.sz);
                                        if ((sz1 == 0) || (sz1 != d.data.sz)) {
                                            ERR("FW-write fail");
                                            return;
                                        }
                                    }
                                    updTimeout();
                                    break;
                                    
                                case 0x4a: // fwupd end
                                    if (!Update.end()) {
                                        ERR("FW-finish fail");
                                        return;
                                    }
                                    
                                    NEXT("Wait server fin...", NS_SERVER_DATA_CONFIRM, 3000);
                                    srvSend(0x4c); // fwupd ok
                                    cfg.set().fwupdind = 0;
                                    fwupd = cfg.save();
                                    return;
            
                                default:
                                    ERR("recv unknown cmd");
                                    return;
                            }
                        }
                    }
                    return;
                
                case NS_EXIT:
                    // ожидание таймаута сообщения перед выходом из режима синхронизации
                    if (timeout < millis())
                        close();
                    if (fwupd)
                        ESP.restart();
                    return;
                
                default:
                    close();
                    return;
            }
        }
        
        void btnSmpl(btn_code_t btn) {
            // короткое нажатие на любую кнопку ускоряет выход,
            // если процес завершился и ожидается таймаут финального сообщения
            if (state == NS_EXIT)
                close();
        }
        
        bool useLong(btn_code_t btn) {
            return btn == BTN_SEL;
        }
        void btnLong(btn_code_t btn) {
            // длинное нажатие из любой стадии завершает процесс синхнонизации принудительно
            if (btn != BTN_SEL)
                return;
            close();
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            u8g2.setFont(u8g2_font_ncenB08_tr);
    
            // Заголовок
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,128,12);
            u8g2.setDrawColor(0);
            char s[20];
            strcpy_P(s, PSTR("Web Sync"));
            u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
    
            strcpy_P(s, PSTR("WiFi"));
            u8g2.drawStr(0, y, s);
            switch (WiFi.status()) {
                case WL_NO_SHIELD:      strcpy_P(s, PSTR("No shield")); break;
                case WL_IDLE_STATUS:    strcpy_P(s, PSTR("Idle")); break;
                case WL_NO_SSID_AVAIL:  strcpy_P(s, PSTR("SSID not avail")); break;
                case WL_SCAN_COMPLETED: strcpy_P(s, PSTR("Scan completed")); break;
                case WL_CONNECTED:
                    strncpy(s, WiFi.SSID().c_str(), sizeof(s));
                    s[sizeof(s)-1] = '\0';
                    break;
                case WL_CONNECT_FAILED: strcpy_P(s, PSTR("Connect fail")); break;
                case WL_CONNECTION_LOST:strcpy_P(s, PSTR("Connect lost")); break;
                case WL_DISCONNECTED:   strcpy_P(s, PSTR("Disconnected")); break;
            }
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
            y += 10;
            if (state == NS_PROFILE_JOIN) {
                strcpy_P(s, PSTR("Wait to JOIN"));
                u8g2.drawStr(0, y, s);
                snprintf_P(s, sizeof(s), PSTR("%d sec"), (timeout-millis()) / 1000);
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
                y += 25;
                u8g2.setFont(u8g2_font_fub20_tr);
                snprintf_P(s, sizeof(s), PSTR("%04X"), joinnum);
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
                return;
            }
    
            if (WiFi.status() == WL_CONNECTED) {
                snprintf_P(s, sizeof(s), PSTR("%3d dBm"), WiFi.RSSI());
                u8g2.drawStr(0, y, s);
                const auto &ip = WiFi.localIP();
                snprintf_P(s, sizeof(s), PSTR("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
            }
    
            y += 10;
            u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(title))/2, y, title);
    
            y += 10;
            if ((WiFi.status() != WL_NO_SHIELD) && (WiFi.status() != WL_IDLE_STATUS)) {
                uint16_t t = (millis() & 0x3FFF) >> 10;
                s[t] = '\0';
                while (t > 0) {
                    s[t-1] = '.';
                    t--;
                }
                //u8g2.drawStr(0, y, s);
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            }
        }
        
    private:
        char title[24], ssid[40], pass[40];
        uint32_t timeout;
        uint16_t joinnum = 0;
        netsync_state_t state = NS_NONE;
        bool fwupd = false;
};
ViewNetSync vNetSync;



/* ------------------------------------------------------------------------------------------- *
 *  ViewMenuWifiSync - список wifi-сетей
 * ------------------------------------------------------------------------------------------- */
typedef struct {
    char name[33];
    char txt[40];
    bool isopen;
} wifi_t;

class ViewMenuWifiSync : public ViewMenu {
    public:
        void restore() {
            CONSOLE("Wifi init begin");
            CONSOLE("wifi power 1: %d", wifiPower());
            if (!wifiStart())
                return;
            CONSOLE("wifi power 2: %d", wifiPower());
            
            uint16_t n = wifiScan();
            CONSOLE("wifi power 3: %d", wifiPower());
            CONSOLE("scan: %d", n);
            setSize(n);
            
            ViewMenu::restore();
        }
        
        void close() {
            wifiall.clear();
            setSize(0);
            wifiStop();
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            CONSOLE("ViewMenuWifiSync::getStr: %d (sz=%d)", i, wifiall.size());
            auto n = wifiScanInfo(i);
            if (n == NULL) {
                str.name[0] = '\0';
                str.val[0] = '\0';
                return;
            }
            
            snprintf_P(
                str.name, sizeof(str.name), 
                PSTR("%s (%d) %c"), n->ssid, n->rssi, n->isopen ? ' ':'*'
            );
            
            if (n->isopen) {
                str.val[0] = '\0';
            }
            else {
                str.val[0] = wifiPassFind(n->ssid) ? '+' : 'x';
            }
            str.val[1] = '\0';
        }
        
        void btnSmpl(btn_code_t btn) {
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel)) {
                if (btn == BTN_SEL) close();
                ViewMenu::btnSmpl(btn);
                return;
            }

            auto const &w = wifiall[sel()];
            if (w.isopen) {
                viewSet(vNetSync);
                vNetSync.begin(w.name, NULL);
            }
            else {
                char pass[64];
                if (!wifiPassFind(w.name, pass)) {
                    menuFlashP(PSTR("Password required!"));
                    return;
                }
                
                viewSet(vNetSync);
                vNetSync.begin(w.name, pass);
            }
        }
        
        void process() {
            CONSOLE("wifi power p: %d", wifiPower());
            if (btnIdle() > MENU_TIMEOUT) {
                close();
                setViewMain();
            }
        }
        
    private:
        std::vector<wifi_t> wifiall;
};

static ViewMenuWifiSync vMenuWifiSync;
ViewMenu *menuWifiSync() { return &vMenuWifiSync; }
