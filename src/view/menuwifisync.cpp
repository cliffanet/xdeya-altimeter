
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../core/filetxt.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../net/srv.h"
#include "../net/data.h"
#include "../net/wifi.h"
#include "../net/wifisync.h"

#include <Update.h>         // Обновление прошивки
#include <vector>
#include <lwip/inet.h>      // htonl



/* ------------------------------------------------------------------------------------------- *
 *  Поиск пароля по имени wifi-сети
 * ------------------------------------------------------------------------------------------- */
/*
static bool wifiPassRemove() {
    if (!fileExists(PSTR(WIFIPASS_FILE)))
        return true;
    return fileRemove(PSTR(WIFIPASS_FILE));
}
*/
static bool wifiPassFind(const char *ssid, char *pass = NULL) {
    size_t len = strlen(ssid);
    char ssid1[len+1];
    FileTxt f(PSTR(WIFIPASS_FILE));
    
    CONSOLE("need ssid: %s (avail: %d)", ssid, f.available());
    
    if (!f)
        return false;
    
    while (f.available() > 0) {
        if (!f.find_param(PSTR("ssid")))
            break;
        f.read_line(ssid1, sizeof(ssid1));
        if (strcmp(ssid1, ssid) != 0)
            continue;
        
        char param[30];
        f.read_param(param, sizeof(param));
        if (strcmp_P(param, PSTR("pass")) == 0)
            f.read_line(pass, 33);
        else
        if (pass != NULL)
            *pass = 0;
        if (pass != NULL)
            CONSOLE("found pass: %s", pass);
        else
            CONSOLE("founded");
        f.close();
        return true;
    }
    
    CONSOLE("EOF");

    f.close();
    return false;
}

/*
static bool verAvailRemove() {
    if (!fileExists(PSTR(VERAVAIL_FILE)))
        return true;
    return fileRemove(PSTR(VERAVAIL_FILE));
}
*/

/* ------------------------------------------------------------------------------------------- *
 *  ViewNetSync - процесс синхронизации с сервером
 * ------------------------------------------------------------------------------------------- */
/*
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

static size_t progress_max = 0, progress_val = 0;
void netsyncProgMax(size_t max) {
    progress_max = max;
    progress_val = 0;
}
void netsyncProgInc(size_t val) {
    if (progress_max <= 0)
        return;
    progress_val += val;
    
    displayUpdate();
}

#define MSG(s)              msg(PSTR(TXT_WIFI_MSG_ ## s))
#define NEXT(s, h, tout)    next(PSTR(TXT_WIFI_NEXT_ ## s), h, tout)
#define ERR(s)              fin(PSTR(TXT_WIFI_ERR_ ## s))
#define FIN(s)              fin(PSTR(TXT_WIFI_ ## s))

class ViewNetSync : public ViewBase {
    public:
        // старт процедуры синхронизации
        void begin(const char *_ssid, const char *_pass) {
            snprintf_P(title, sizeof(title), PTXT(WIFI_CONNECTTONET), _ssid);
            setState(NS_WIFI_CONNECT, 200);
            joinnum = 0;
            netsyncProgMax(0);
    
            CONSOLE("wifi to: %s; pass: %s", _ssid, _pass == NULL ? "-no-" : _pass);
            if (!wifiStart()) {
                ERR(WIFIINIT);
                return;
            }
            if (!wifiConnect(_ssid, _pass)) {
                ERR(WIFICONNECT);
                return;
            }
        }
        
        void updTimeout(int16_t _timeout = 100) {
            if (_timeout > 0)
                timeout = 1 + _timeout;
            CONSOLE("timeout: %d", _timeout);
        }
        
        // изменение этапа
        void setState(netsync_state_t _state, int32_t _timeout = -1) {
            state = _state;
            updTimeout(_timeout);
            netsyncProgMax(0);
            CONSOLE("state: %d", _state);
        }
        
        // вывод сообщения
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
        void next(const char *_title, netsync_state_t _state, int32_t _timeout = -1) {
            msg(_title);
            setState(_state, _timeout);
        }
        
        // завершение всего процесса, но выход не сразу, а через несколько сек,
        // чтобы прочитать сообщение
        void fin(const char *_title) {
            f.close();
            fileExtStop();
            srvStop();
            wifiStop();
            next(_title, NS_EXIT, 70);
        }
        
        // завершение всего процесса синхронизации с мнгновенным переходом в главный экран
        void close() {
            f.close();
            fileExtStop();
            srvStop();
            wifiStop();
            netsyncProgMax(0);
            setViewMain();
        }
        
        // проверка, всё ли нормально с сервером
        bool chksrv() {
            const char *e = srvErr(); // ошибка это строка из progmem
            if (e != NULL) {
                fin(e);
                return false;
            }
            if (timeout == 1) {
                ERR(TIMEOUT);
                return false;
            }
            return true;
        }
        
        // старт авторизации на сервере
        void authStart() {
            ConfigWebJoin wjoin;
            if (!wjoin.load()) {
                ERR(JOINLOAD);
                return;
            }

            CONSOLE("[authStart] authid: %lu", wjoin.authid());

            uint32_t id = htonl(wjoin.authid());
            if (!srvSend(0x01, id)) {
                ERR(SENDAUTH);
                return;
            }
            
            NEXT(HELLO, NS_SERVER_HELLO, 50);
        }
        
        // пересылка данных на сервер
        void dataToServer(const daccept_t &acc) {
            CONSOLE("dataToServer");
    
            if ((acc.ckscfg == 0) || (acc.ckscfg != cfg.chksum())) {
                MSG(SENDCONFIG);
                if (!sendCfg()) {
                    ERR(SENDCONFIG);
                    return;
                }
            }
    
            if ((acc.cksjmp == 0) || (acc.cksjmp != jmp.chksum())) {
                MSG(SENDJUMPCOUNT);
                if (!sendJump()) {
                    ERR(SENDJUMPCOUNT);
                    return;
                }
            }
    
            if ((acc.ckspnt == 0) || (acc.ckspnt != pnt.chksum())) {
                MSG(SENDPOINT);
                if (!sendPoint()) {
                    ERR(SENDPOINT);
                    return;
                }
            }
    
            MSG(SENDLOGBOOK);
            if (!sendLogBook(acc.ckslog, acc.poslog)) {
                ERR(SENDLOGBOOK);
                return;
            }
    
            MSG(SENDTRACK);
            fileExtInit();
            if (!sendTrack(acc.ckstrack)) {
                ERR(SENDTRACK);
                return;
            }
            
            MSG(SENDFIN);
            if (!sendDataFin()) {
                ERR(SENDFIN);
                return;
            }
    
            NEXT(CONFIRM, NS_SERVER_DATA_CONFIRM, 300);
        }
        
        // фоновый процессинг - обрабатываем этапы синхронизации
        void process() {
            if (timeout > 1)
                timeout --;
            
            switch (state) {
                case NS_WIFI_CONNECT:
                // этап 1 - ожидаем соединения по вифи
                    if (wifiStatus() == WIFI_STA_CONNECTED) {
                        CONSOLE("wifi ok, try to server connect");
                        // вифи подключилось, соединяемся с сервером
                        if (!srvConnect()) {
                            ERR(SERVERCONNECT);
                            return;
                        }
                        
                        // авторизаемся на сервере
                        authStart();
                        return;
                    }
                    
                    // проблемы при соединении с вифи
                    if (wifiStatus() == WIFI_STA_FAIL) {
                        ERR(WIFICONNECT);
                        return;
                    }
    
                    if (timeout == 1)
                        ERR(WIFITIMEOUT);
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
                                setState(NS_PROFILE_JOIN, 1200);
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
    
                        ERR(RCVCMDUNKNOWN);
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
                                srvSend(0x12, htonl(timeout * 100));
                                n=0;
                            }
                            return;
                        }
    
                        if (cmd != 0x13) {
                            ERR(RCVCMDUNKNOWN);
                            return;
                        }
    
                        d.authid = ntohl(d.authid);
                        d.secnum = ntohl(d.secnum);
    
                        CONSOLE("[waitJoin] cmd: %02x (%lu, %lu)", cmd, d.authid, d.secnum);
    
                        ConfigWebJoin wjoin(d.authid, d.secnum);
                        if (!wjoin.save()) {
                            ERR(SAVEJOIN);
                            return;
                        }
    
                        if (!srvSend(0x14)) {
                            ERR(SENDJOIN);
                            return;
                        }
    
                        joinnum = 0;
                        MSG(JOINACCEPT);
    
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
                                if (!wifiPassRemove()) {
                                    ERR(PASSCLEAR);
                                    return;
                                }
                                if (!f.open_P(PSTR(WIFIPASS_FILE), FileMy::MODE_APPEND)) {
                                    ERR(PASSCREATE);
                                    return;
                                }
                                NEXT(RCVPASS, NS_RCV_WIFI_PASS, 30);
                                return;
                                
                            case 0x44: // veravail beg
                                if (!verAvailRemove()) {
                                    ERR(VERAVAILCLEAR);
                                    return;
                                }
                                if (!f.open_P(PSTR(VERAVAIL_FILE), FileMy::MODE_APPEND)) {
                                    ERR(VERAVAILCREATE);
                                    return;
                                }
                                NEXT(RCVFWVER, NS_RCV_VER_AVAIL, 30);
                                return;
                                
                            case 0x47: // firmware update beg
                                NEXT(FWUPDATE, NS_RCV_FWUPDATE, 30);
                                return;
        
                            case 0x0f: // bye
                                FIN(SYNCFINISHED);
                                return;
                        }
    
                        ERR(RCVCMDUNKNOWN);
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
                                    CONSOLE("add wifi: {%s}, {%s}", ssid, pass);
                                    if (
                                            !FileTxt(f).print_param(PSTR("ssid"), ssid) ||
                                            !FileTxt(f).print_param(PSTR("pass"), pass)
                                        ) {
                                        ERR(PASSADD);
                                        return;
                                    }
                                    updTimeout();
                                    break;
            
                                case 0x43: // wifi end
                                    if (!f.open_P(PSTR(WIFIPASS_FILE))) {
                                        ERR(PASSADD);
                                        return;
                                    }
                                    cks = FileTxt(f).chksum();
                                    f.close();
                                    cks = htonl(cks);
                                    NEXT(FIN, NS_SERVER_DATA_CONFIRM, 30);
                                    srvSend(0x4a, cks); // wifiok
                                    return;
            
                                default:
                                    ERR(RCVCMDUNKNOWN);
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
                                    CONSOLE("add veravail: {%s}", ver);
                                    if (!FileTxt(f).print_line(ver)) {
                                        ERR(VERAVAILADD);
                                        return;
                                    }
                                    updTimeout();
                                    break;
            
                                case 0x46: // veravail end
                                    if (!f.open_P(PSTR(VERAVAIL_FILE))) {
                                        ERR(VERAVAILADD);
                                        return;
                                    }
                                    cks = FileTxt(f).chksum();
                                    f.close();
                                    cks = htonl(cks);
                                    NEXT(FIN, NS_SERVER_DATA_CONFIRM, 30);
                                    srvSend(0x4b, cks); // veravail ok
                                    return;
            
                                default:
                                    ERR(RCVCMDUNKNOWN);
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
                                            ERR(FWSIZEBIG);
                                            return;
                                        }
            
                                        // start burn
                                        if (!Update.begin(d.info.size, U_FLASH) || !Update.setMD5(d.info.md5)) {
                                            CONSOLE("Upd begin fail: errno=%d", Update.getError());
                                            ERR(FWINIT);
                                            return;
                                        }
                                        
                                        netsyncProgMax(d.info.size);
                                    }
                                    break;
                                    
                                case 0x49: // fwupd data
                                    {
                                        d.data.sz = ntohs(d.data.sz);
                                        int sz1 = Update.write(d.data.buf, d.data.sz);
                                        if ((sz1 == 0) || (sz1 != d.data.sz)) {
                                            ERR(FWWRITE);
                                            return;
                                        }
                                        
                                        netsyncProgInc(d.data.sz);
                                    }
                                    updTimeout();
                                    break;
                                    
                                case 0x4a: // fwupd end
                                    if (!Update.end()) {
                                        ERR(FWFIN);
                                        return;
                                    }
                                    
                                    NEXT(FIN, NS_SERVER_DATA_CONFIRM, 30);
                                    srvSend(0x4c); // fwupd ok
                                    cfg.set().fwupdind = 0;
                                    fwupd = cfg.save();
                                    netsyncProgMax(0);
                                    return;
            
                                default:
                                    ERR(RCVCMDUNKNOWN);
                                    return;
                            }
                        }
                    }
                    return;
                
                case NS_EXIT:
                    // ожидание таймаута сообщения перед выходом из режима синхронизации
                    if (timeout == 1)
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
            u8g2.setFont(menuFont);
    
            // Заголовок
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
            u8g2.setDrawColor(0);
            char s[64];
            strcpy_P(s, PTXT(WIFI_WEBSYNC));
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
            
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
            
            strcpy_P(s, PTXT(WIFI_NET));
            u8g2.drawTxt(0, y, s);
            
            int8_t rssi;
            switch (wifiStatus()) {
                case WIFI_STA_NULL:         strcpy_P(s, PTXT(WIFI_STATUS_OFF));         break;
                case WIFI_STA_DISCONNECTED: strcpy_P(s, PTXT(WIFI_STATUS_DISCONNECT));  break;
                case WIFI_STA_FAIL:         strcpy_P(s, PTXT(WIFI_STATUS_FAIL));        break;
                case WIFI_STA_WAITIP:
                case WIFI_STA_CONNECTED:
                    wifiInfo(s, rssi);
                    break;
            }
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
            
            y += 10;
            if (state == NS_PROFILE_JOIN) {
                strcpy_P(s, PTXT(WIFI_WAITJOIN));
                u8g2.drawTxt(0, y, s);
                snprintf_P(s, sizeof(s), PTXT(WIFI_WAIT_SEC), timeout / 10);
                u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
                y += 25;
                u8g2.setFont(u8g2_font_fub20_tr);
                snprintf_P(s, sizeof(s), PSTR("%04X"), joinnum);
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
                return;
            }

            if ((wifiStatus() == WIFI_STA_WAITIP) || (wifiStatus() == WIFI_STA_CONNECTED)) {
                snprintf_P(s, sizeof(s), PTXT(WIFI_RSSI), rssi);
                u8g2.drawTxt(0, y, s);
                
                if (wifiStatus() == WIFI_STA_CONNECTED) {
                    const auto &ip = wifiIP();
                    snprintf_P(s, sizeof(s), PSTR("%d.%d.%d.%d"), ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);
                    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
                }
            }
            
            y += 10;
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(title))/2, y, title);

            y += 10;
            if (wifiStatus() > WIFI_STA_NULL) {
                if (progress_max > 0) {
                    uint8_t p = progress_val * 20 / progress_max;
                    char *ss = s;
                    *ss = '|';
                    ss++;
                    for (uint8_t i=0; i< 20; i++, ss++)
                        *ss = i < p ? '.' : ' ';
                    *ss = '|';
                    ss++;
                    *ss = '\0';
                }
                else {
                    uint16_t t = (millis() & 0x3FFF) >> 10;
                    s[t] = '\0';
                    while (t > 0) {
                        s[t-1] = '.';
                        t--;
                    }
                }
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            }
        }
        
    private:
#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
        char title[50];
#else
        char title[24];
#endif
        char ssid[40], pass[40];
        uint16_t timeout;
        uint16_t joinnum = 0;
        netsync_state_t state = NS_NONE;
        bool fwupd = false;
        FileMy f;
};
ViewNetSync vNetSync;
*/



class ViewNetSync2 : public ViewBase {
    public:
        void btnSmpl(btn_code_t btn) {
            // короткое нажатие на любую кнопку ускоряет выход,
            // если процес завершился и ожидается таймаут финального сообщения
            wifiSyncStop();
            // Переход в ViewMain будет и так автоматически в методе draw(),
            // как только закончится процесс wifiSync, но это будет с задержкой.
            // Чтобы переключение произошло сразу, сделаем его тут
            //setViewMain();
        }
        
        /*
        bool useLong(btn_code_t btn) {
            if (btn != BTN_SEL)
                return false;
            
            auto *w = wifiSyncProc();
            return (w != NULL) && w->isrun();
        }
        void btnLong(btn_code_t btn) {
            // длинное нажатие из любой стадии завершает процесс синхнонизации принудительно
            if (btn != BTN_SEL)
                return;
            auto *w = wifiSyncProc();
            if (w != NULL)
                w->stop();
        }
        */
        
        void drawTitle(U8G2 &u8g2, int y, const char *txt_P) {
            char title[60];
            strncpy_P(title, txt_P, sizeof(title));
            title[sizeof(title)-1] = 0;
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(title))/2, y, title);
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            auto st = wifiSyncState();
            /*
            if (w == NULL) {
                setViewMain();
                return;
            }
            */
            
            u8g2.setFont(menuFont);
    
            // Заголовок
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
            u8g2.setDrawColor(0);
            char s[64];
            strcpy_P(s, PTXT(WIFI_WEBSYNC));
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
            
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
            
            /*
            strcpy_P(s, PTXT(WIFI_NET));
            u8g2.drawTxt(0, y, s);
            
            int8_t rssi;
            switch (wifiStatus()) {
                case WIFI_STA_NULL:         strcpy_P(s, PTXT(WIFI_STATUS_OFF));         break;
                case WIFI_STA_DISCONNECTED: strcpy_P(s, PTXT(WIFI_STATUS_DISCONNECT));  break;
                case WIFI_STA_FAIL:         strcpy_P(s, PTXT(WIFI_STATUS_FAIL));        break;
                case WIFI_STA_WAITIP:
                case WIFI_STA_CONNECTED:
                    wifiInfo(s, rssi);
                    break;
            }
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
            
            y += 10;
            
            if (w->op() == WorkerWiFiSync::opProfileJoin) {
                strcpy_P(s, PTXT(WIFI_WAITJOIN));
                u8g2.drawTxt(0, y, s);
                snprintf_P(s, sizeof(s), PTXT(WIFI_WAIT_SEC), w->timer() / 10);
                u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
                y += 25;
                u8g2.setFont(u8g2_font_fub20_tr);
                snprintf_P(s, sizeof(s), PSTR("%04X"), w->joinnum());
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
                return;
            }

            if ((wifiStatus() == WIFI_STA_WAITIP) || (wifiStatus() == WIFI_STA_CONNECTED)) {
                snprintf_P(s, sizeof(s), PTXT(WIFI_RSSI), rssi);
                u8g2.drawTxt(0, y, s);
                
                if (wifiStatus() == WIFI_STA_CONNECTED) {
                    const auto &ip = wifiIP();
                    snprintf_P(s, sizeof(s), PSTR("%d.%d.%d.%d"), ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);
                    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
                }
            }
            */
            
            y += 10;
#define TITLE(txt)      drawTitle(u8g2, y, PSTR(TXT_WIFI_ ## txt)); break;
            switch (st) {
                case wSync::stNotRun:           TITLE(MSG_NOTRUN);
                case wSync::stWiFiInit:         TITLE(MSG_WIFIINIT);
                case wSync::stWiFiConnect:      TITLE(MSG_WIFICONNECT);
                case wSync::stWiFiWait:         TITLE(MSG_SRVAUTH);
                case wSync::stFinOk:            TITLE(SYNCFINISHED);
                case wSync::stUserCancel:       TITLE(MSG_USERCANCEL);
                case wSync::errWiFiInit:        TITLE(ERR_WIFIINIT);
                case wSync::errWiFiConnect:     TITLE(ERR_WIFICONNECT);
                case wSync::errTimeout:         TITLE(ERR_TIMEOUT);
                case wSync::errSrvConnect:      TITLE(ERR_SERVERCONNECT);
                case wSync::errSrvDisconnect:   TITLE(ERR_SRVDISCONNECT);
                case wSync::errRecvData:        TITLE(ERR_RCVDATA);
                case wSync::errRcvCmdUnknown:   TITLE(ERR_RCVCMDUNKNOWN);
                case wSync::errSendData:        TITLE(ERR_SENDDATA);
                case wSync::errJoinLoad:        TITLE(ERR_JOINLOAD);
                case wSync::errJoinSave:        TITLE(ERR_SAVEJOIN);
                case wSync::errWorker:          TITLE(ERR_WORKER);
                
                /*
                case WorkerWiFiSync::opWiFiConnect:     
                case WorkerWiFiSync::opSrvConnect:      TITLE(MSG_SRVCONNECT);
                case WorkerWiFiSync::opSrvAuth:
                case WorkerWiFiSync::opWaitAuth:        TITLE(MSG_SRVAUTH);
                case WorkerWiFiSync::opSndConfig:       TITLE(MSG_SENDCONFIG);
                case WorkerWiFiSync::opSndJumpCount:    TITLE(MSG_SENDJUMPCOUNT);
                case WorkerWiFiSync::opSndPoint:        TITLE(MSG_SENDPOINT);
                case WorkerWiFiSync::opSndLogBook:      TITLE(MSG_SENDLOGBOOK);
                case WorkerWiFiSync::opSndDataFin:      TITLE(MSG_SENDFIN);
                */
            }
#undef TITLE
            
            /*
            y += 10;
            if (wifiStatus() > WIFI_STA_NULL) {
                if (progress_max > 0) {
                    uint8_t p = progress_val * 20 / progress_max;
                    char *ss = s;
                    *ss = '|';
                    ss++;
                    for (uint8_t i=0; i< 20; i++, ss++)
                        *ss = i < p ? '.' : ' ';
                    *ss = '|';
                    ss++;
                    *ss = '\0';
                }
                else {
                    uint16_t t = (millis() & 0x3FFF) >> 10;
                    s[t] = '\0';
                    while (t > 0) {
                        s[t-1] = '.';
                        t--;
                    }
                }
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            }
            */
        }
};
ViewNetSync2 vNetSync;


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
            // Сначала прорисуем на экране, что мы начали сканировать
            _isinit = true;
            setSize(2);
            ViewMenu::restore();
            displayUpdate();
            
            // А теперь начнём сканировать
            CONSOLE("wifi power 1: %d", wifiPower());
            if (!wifiStart())
                return;
            CONSOLE("wifi power 2: %d", wifiPower());
            
            uint16_t n = wifiScan();
            CONSOLE("wifi power 3: %d", wifiPower());
            CONSOLE("scan: %d", n);
            setSize(n);
            wifiStop();

            _isinit = false;
            
            // И снова обновим список отображаемых пунктов
            ViewMenu::restore();
        }
        
        void close() {
            setSize(0);
            wifiScanClean();
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            if (_isinit) {
                switch (i) {
                    case 0:
                        str.name[0] = '\0';
                        str.val[0] = '\0';
                        return;
                        
                    case 1:
                        strcpy_P(str.name, PTXT(WIFI_SEARCHNET));
                        str.val[0] = '\0';
                        return;
                }
                return;
            }
            
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

            auto n = wifiScanInfo(sel());
            if (n == NULL)
                return;
            
            if (n->isopen) {
                wifiSyncBegin(n->ssid);
                viewSet(vNetSync);
                //vNetSync.begin(n->ssid, NULL);
            }
            else {
                char pass[64];
                if (!wifiPassFind(n->ssid, pass)) {
                    menuFlashP(PTXT(WIFI_NEEDPASSWORD));
                    return;
                }

                wifiSyncBegin(n->ssid, pass);
                viewSet(vNetSync);
                //vNetSync.begin(n->ssid, pass);
            }
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT) {
                close();
                setViewMain();
            }
        }
        
    private:
        std::vector<wifi_t> wifiall;
        bool _isinit = true;
};

static ViewMenuWifiSync vMenuWifiSync;
ViewMenu *menuWifiSync() { return &vMenuWifiSync; }

bool menuIsWifi() {
    return viewIs(vMenuWifiSync) || viewIs(vNetSync);
}
