
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../core/filetxt.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../net/wifi.h"
#include "../net/wifisync.h"
#include "../net/wifiapp.h"

#include <vector>



/* ------------------------------------------------------------------------------------------- *
 *  Поиск пароля по имени wifi-сети
 * ------------------------------------------------------------------------------------------- */
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


class ViewNetSync : public ViewBase {
    public:
        void btnSmpl(btn_code_t btn) {
            // короткое нажатие на любую кнопку ускоряет выход,
            // если процес завершился и ожидается таймаут финального сообщения
            if (!wifiSyncIsRun()) {
                wifiSyncDelete();
                setViewMain();
            }
        }
        
        bool useLong(btn_code_t btn) {
            if (btn != BTN_SEL)
                return false;
            
            return wifiSyncIsRun();
        }
        void btnLong(btn_code_t btn) {
            // длинное нажатие из любой стадии завершает процесс синхнонизации принудительно
            if (btn != BTN_SEL)
                return;
            wifiSyncStop();
        }
        
        void drawTitle(U8G2 &u8g2, int y, const char *txt_P) {
            char title[60];
            strncpy_P(title, txt_P, sizeof(title));
            title[sizeof(title)-1] = 0;
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(title))/2, y, title);
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            wSync::info_t inf;
            auto st = wifiSyncState(inf);
            
            if (st == wSync::stNotRun) {
                setViewMain();
                return;
            }
            
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
            
            if (st == wSync::stProfileJoin) {
                strcpy_P(s, PTXT(WIFI_WAITJOIN));
                u8g2.drawTxt(0, y, s);
                snprintf_P(s, sizeof(s), PTXT(WIFI_WAIT_SEC), inf.timeout / 10);
                u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
                y += 25;
                u8g2.setFont(u8g2_font_fub20_tr);
                snprintf_P(s, sizeof(s), PSTR("%04X"), inf.joinnum);
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
#define TITLE(txt)      drawTitle(u8g2, y, PSTR(TXT_WIFI_ ## txt)); break;
            switch (st) {
                case wSync::stWiFiInit:         TITLE(MSG_WIFIINIT);
                case wSync::stWiFiConnect:      TITLE(MSG_WIFICONNECT);
                case wSync::stSrvConnect:       TITLE(MSG_SRVCONNECT);
                case wSync::stSrvAuth:          TITLE(MSG_SRVAUTH);
                case wSync::stSendConfig:       TITLE(MSG_SENDCONFIG);
                case wSync::stSendJumpCount:    TITLE(MSG_SENDJUMPCOUNT);
                case wSync::stSendPoint:        TITLE(MSG_SENDPOINT);
                case wSync::stSendLogBook:      TITLE(MSG_SENDLOGBOOK);
                case wSync::stSendTrackList:
                case wSync::stSendTrack:        TITLE(MSG_SENDTRACK);
                case wSync::stSendDataFin:      TITLE(MSG_SENDFIN);
                case wSync::stRecvWiFiPass:     TITLE(NEXT_RCVPASS);
                case wSync::stRecvVerAvail:     TITLE(NEXT_RCVFWVER);
                case wSync::stRecvFirmware:     TITLE(NEXT_FWUPDATE);
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
            }
#undef TITLE
            
            y += 10;
            if (wifiStatus() > WIFI_STA_NULL) {
                if (inf.cmplsz > 0) {
                    uint8_t p = inf.cmplval * 20 / inf.cmplsz;
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
};
ViewNetSync vNetSync;


/* ------------------------------------------------------------------------------------------- *
 *  ViewMenuWifiNet - список wifi-сетей
 *  вызываем doafter, чтобы выбрать нужную
 * ------------------------------------------------------------------------------------------- */
typedef struct {
    char name[33];
    char txt[40];
    bool isopen;
} wifi_t;

class ViewMenuWifiNet : public ViewMenu {
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
                netopen(n->ssid);
            }
            else {
                char pass[64];
                if (!wifiPassFind(n->ssid, pass)) {
                    menuFlashP(PTXT(WIFI_NEEDPASSWORD));
                    return;
                }
                
                netopen(n->ssid, pass);
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
        
        virtual void netopen(const char *ssid, const char *pass = NULL) = 0;
};

class ViewMenuWifi2Cli : public ViewMenuWifiNet {
    void netopen(const char *ssid, const char *pass = NULL) {
        wifiCliBegin(ssid, pass);
        setViewMain();
    }
};
static ViewMenuWifi2Cli vMenuWifi2Cli;
ViewMenu *menuWifi2Cli() { return &vMenuWifi2Cli; }

class ViewMenuWifi2Server : public ViewMenuWifiNet {
    void netopen(const char *ssid, const char *pass = NULL) {
        wifiSyncBegin(ssid, pass);
        viewSet(vNetSync);
    }
};
static ViewMenuWifi2Server vMenuWifi2Server;
ViewMenu *menuWifi2Server() { return &vMenuWifi2Server; }

bool menuIsWifi() {
    return viewIs(vMenuWifi2Cli) || viewIs(vMenuWifi2Server) || viewIs(vNetSync);
}
