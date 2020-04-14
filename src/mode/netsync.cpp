/*
    Network processing for Web-sync
*/

#include "../mode.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"
#include "../menu/base.h"
#include "../button.h"
#include "../display.h"
#include "../net/srv.h"
#include "../net/data.h"

#include <WiFi.h> // htonl

static uint32_t timeout;
static char title[20];
static uint16_t joinnum = 0;

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayNetSync(U8G2 &u8g2) {
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
    if (joinnum > 0) {
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

uint32_t netSyncTimeout() { return timeout; }

/* ------------------------------------------------------------------------------------------- *
 *  Выход с паузой перед этим, чтобы увидеть результат синхронизации
 * ------------------------------------------------------------------------------------------- */
static void syncExit() {
    srvStop();
    WiFi.mode(WIFI_OFF);
    hndProcess = NULL;
    modeMain();
}

static void waitToExit() {
    if (timeout >= millis())
        return;
    syncExit();
}

#define ERR(s) toExit(PSTR(s))
void toExit(const char *_title) {
    if (_title == NULL)
        title[0] = '\0';
    else {
        strncpy_P(title, _title, sizeof(title));
        title[sizeof(title)-1] = '\0';
        Serial.printf("toExit: %s\r\n", title);
    }
    srvStop();
    WiFi.mode(WIFI_OFF);
    hndProcess = waitToExit;
    timeout = millis() + 5000;
    joinnum = 0;
}

//#define MSG(s) msg(PSTR(s))
//#define MSG(s, h) msg(PSTR(s), h)
#define MSG(s, h, tout) msg(PSTR(s), h, tout)
static void msg(const char *_title = NULL, void (*hnd)() = NULL, int32_t _timeout = -1) {
    if (_title == NULL)
        title[0] = '\0';
    else {
        strncpy_P(title, _title, sizeof(title));
        title[sizeof(title)-1] = '\0';
        Serial.printf("msg: %s\r\n", title);
    }
    if (hnd != NULL)
        hndProcess = hnd;
    if (_timeout > 0) {
        timeout = millis() + _timeout;
    }
    displayUpdate();
}

/* ------------------------------------------------------------------------------------------- *
 *  Ожидание завершения
 * ------------------------------------------------------------------------------------------- */
static void waitFin() {
    uint8_t cmd;
    if (!srvRecv(cmd))
        return;
    
    Serial.printf("[waitFin] cmd: %02x\r\n", cmd);
    
    switch (cmd) {
        case 0x0f: // bye
            toExit(PSTR("Sync finished"));
            return;
    }
    
    ERR("recv unknown cmd");
}

/* ------------------------------------------------------------------------------------------- *
 *  Пересылка на сервер данных
 * ------------------------------------------------------------------------------------------- */
static void dataToServer(const daccept_t &acc) {
    Serial.println("dataToServer");
    
    if ((acc.ckscfg == 0) || (acc.ckscfg != cfg.chksum())) {
        msg(PSTR("Sending config..."));
        if (!sendCfg()) {
            ERR("send cfg fail");
            return;
        }
    }
    
    if ((acc.cksjmp == 0) || (acc.cksjmp != jmp.chksum())) {
        msg(PSTR("Sending jump count..."));
        if (!sendJump()) {
            ERR("send cmp count fail");
            return;
        }
    }
    
    if ((acc.ckspnt == 0) || (acc.ckspnt != pnt.chksum())) {
        msg(PSTR("Sending point..."));
        if (!sendPoint()) {
            ERR("send pnt fail");
            return;
        }
    }
    
    msg(PSTR("Sending LogBook..."));
    if (!sendLogBook(acc.ckslog, acc.poslog)) {
        ERR("send LogBook fail");
        return;
    }
    
    msg(PSTR("Sending Tracks..."));
    if (!sendTrack(acc.ckstrack)) {
        ERR("send Tracks fail");
        return;
    }
    
    MSG("Wait server confirm...", waitFin, 30000);
    srvSend(0x3f);
}

/* ------------------------------------------------------------------------------------------- *
 *  Ожидание кода подключения
 * ------------------------------------------------------------------------------------------- */
static void authStart();
static void waitJoin() {
    uint8_t cmd;
    struct __attribute__((__packed__)) {
        uint32_t authid;
        uint32_t secnum;
    } d;
    if (!srvRecv(cmd, d)) {
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
    
    Serial.printf("[waitJoin] cmd: %02x (%lu, %lu)\r\n", cmd, d.authid, d.secnum);
    
    ConfigWebJoin wjoin(d.authid, d.secnum);
    if (!wjoin.save()) {
        ERR("save fail");
        return;
    }
    
    if (!srvSend(0x14))
        return;
    
    joinnum = 0;
    MSG("join accepted", NULL, -1);
    
    authStart();
}

/* ------------------------------------------------------------------------------------------- *
 *  Ожидание ответа на приветствие
 * ------------------------------------------------------------------------------------------- */
static void waitHello() {
    uint8_t cmd;
    union {
        uint32_t num;
        daccept_t acc;
    } d;
    if (!srvRecv(cmd, d))
        return;
    
    Serial.printf("[waitHello] cmd: %02x\r\n", cmd);
    
    switch (cmd) {
        case 0x10: // rejoin
            joinnum = ntohl(d.num);
            msg(NULL, waitJoin, 120000);
            return;
        
        case 0x20: // accept
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

/* ------------------------------------------------------------------------------------------- *
 *  Запуск авторизации
 * ------------------------------------------------------------------------------------------- */
static void authStart() {
    ConfigWebJoin wjoin;
    if (!wjoin.load()) {
        ERR("Can\'t load JOIN-inf");
        return;
    }
    
    Serial.printf("[authStart] authid: %lu\r\n", wjoin.authid());
    
    uint32_t id = htonl(wjoin.authid());
    if (!srvSend(0x01, id))
        return;
    
    MSG("wait hello", waitHello, 3000);
}

/* ------------------------------------------------------------------------------------------- *
 *  Соединение к серверу
 * ------------------------------------------------------------------------------------------- */
static void hndConnecting() {
    if (!srvConnect()) {
        ERR("server can't connect");
        return;
    }
    
    authStart();
}

/* ------------------------------------------------------------------------------------------- *
 *  Ожидание соединение по wifi
 * ------------------------------------------------------------------------------------------- */
static void waitWiFiConnect() {
    if (WiFi.status() == WL_CONNECTED) {
        MSG("connecting to server", hndConnecting, 3000);
        return;
    }
    
    if ((WiFi.status() == WL_NO_SSID_AVAIL) || (WiFi.status() == WL_CONNECT_FAILED)) {
        ERR("wifi connect fail");
        return;
    }
    
    if (timeout < millis())
        ERR("wifi timeout");
}


/* ------------------------------------------------------------------------------------------- *
 *  Старт синхронизации - меняем обработчики экрана и кнопок
 * ------------------------------------------------------------------------------------------- */
void modeNetSync(const char *net) {
    displayHnd(displayNetSync);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_SEL,     BTN_LONG, syncExit);
    
    char ssid[40];
    strcpy(ssid, net);  // Приходится временно копировать ssid, т.к. оно находится внутри класса menuWifi
                        // а menuClear() удаляет этот класс. Перед этим запускать соединение wifi
                        // тоже нельзя, т.к. при удалении меню модуль вифи отключается.
    menuClear();
    
    snprintf_P(title, sizeof(title), PSTR("wifi to %s"), ssid);
    hndProcess = waitWiFiConnect;
    timeout = millis() + 10000;
    joinnum = 0;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char *)ssid, "12344321");
}
