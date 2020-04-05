/*
    Network processing for Web-sync
*/

#include "../mode.h"
#include "../cfg/webjoin.h"
#include "../menu/base.h"
#include "../button.h"
#include "../display.h"

#include <Arduino.h>
#include <WiFi.h> // htonl

static uint32_t timeout;
static char title[20];
static WiFiClient cli;
static uint16_t joinnum = 0;

typedef struct __attribute__((__packed__)) {
    char    mgc;
    uint8_t cmd;
    int16_t len;
} phdr_t;

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

/* ------------------------------------------------------------------------------------------- *
 *  Выход с паузой перед этим, чтобы увидеть результат синхронизации
 * ------------------------------------------------------------------------------------------- */
static void syncExit() {
    cli.stop();
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
static void toExit(const char *_title = NULL) {
    if (_title == NULL)
        title[0] = '\0';
    else {
        strncpy_P(title, _title, sizeof(title));
        title[sizeof(title)-1] = '\0';
    }
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
    }
    if (hnd != NULL)
        hndProcess = hnd;
    if (_timeout > 0) {
        timeout = millis() + _timeout;
    }
    displayUpdate();
}

/* ------------------------------------------------------------------------------------------- *
 *  чтение инфы от сервера, возвращает true, если есть инфа
 * ------------------------------------------------------------------------------------------- */
static bool srvWait(size_t sz) {
    if (cli.available() >= sz)
        return true;
    
    if (!cli.connected())
        ERR("server connect lost");
    else
    if (timeout < millis())
        ERR("server timeout");
    
    return false;
}

static bool srvReadData(uint8_t *data, uint16_t sz) {
    if (sz == 0)
        return true;
    
    size_t sz1 = cli.read(data, sz);
    if (sz1 != sz) {
        ERR("fail read from server");
        return false;
    }
    
    return true;
}

static bool srvWaitHdr(phdr_t &p) {
    if (!srvWait(sizeof(phdr_t)))
        return false;
    
    if (!srvReadData(reinterpret_cast<uint8_t *>(&p), sizeof(phdr_t)))
        return false;
    if ((p.mgc != '#') || (p.cmd == 0)) {
        ERR("recv proto fail");
        return false;
    }
    
    p.len = ntohs(p.len);
    
    return true;
}

static bool srvRecv(uint8_t &cmd, uint8_t *data = NULL, uint16_t sz = 0) {
    static phdr_t p = { .mgc = '\0', .cmd = 0 };
    if (data == NULL) sz = 0;
    
    // ожидание и чтение заголовка
    if ((p.mgc == '\0') && (p.cmd == 0)) {
        if (!srvWaitHdr(p))
            return false;
        if (p.len < sz) // стираем хвост data, который не будет переписан принимаемыми данными,
                        // Удобно, если после изменения протокола, данные по требуемой команде
                        // от сервера будут более короткими.
                        // Но так же это и лишняя процедура, если буфер подаётся с запасом
            bzero(data+p.len, sz-p.len);
    }
    
    cmd = p.cmd;
    
    // Ожидание данных
    if ((p.len > 0) && !srvWait(p.len))
        return false;
    
    // чтение данных
    if (!srvReadData(data, p.len <= sz ? p.len : sz))
        return false;
    
    // И дочитываем буфер, если это требуется
    if (p.len > sz) {
        uint16_t sz1 = p.len - sz;
        uint8_t buf[sz1];
        if (!srvReadData(buf, sz1))
            return false;
    }
    
    // Обозначаем, что дальше надо принимать заголовок
    p.mgc = '\0';
    p.cmd = 0;
    
    return true;
}

template <typename T>
static bool srvRecv(uint8_t &cmd, T &data) {
    return srvRecv(cmd, reinterpret_cast<uint8_t *>(&data), sizeof(T));
}

/* ------------------------------------------------------------------------------------------- *
 *  отправка на сервер
 * ------------------------------------------------------------------------------------------- */
static bool srvSend(uint8_t cmd, const uint8_t *data = NULL, uint16_t sz = 0) {
    if (!cli.connected()) {
        ERR("server connect lost");
        return false;
    }
    
    uint8_t d[4 + sz] = { '%', cmd };
    uint16_t sz1 = htons(sz);
    memcpy(d+2, &sz1, sizeof(sz1));
    if (sz > 0)
        memcpy(d+4, data, sz);
    
    auto sz2 = cli.write(d, 4 + sz);
    if (sz2 != (4 + sz)) {
        ERR("server send fail");
        return false;
    }
    return true;
}

template <typename T>
static bool srvSend(uint8_t cmd, const T &data) {
    return srvSend(cmd, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
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
    uint32_t num;
    if (!srvRecv(cmd, num))
        return;
    
    Serial.printf("[waitHello] cmd: %02x\r\n", cmd);
    
    switch (cmd) {
        case 0x10:
            joinnum = ntohl(num);
            msg(NULL, waitJoin, 120000);
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
    
    uint32_t id = htonl(wjoin.authid());
    if (!srvSend(0x01, id))
        return;
    
    MSG("wait hello", waitHello, 3000);
}

/* ------------------------------------------------------------------------------------------- *
 *  Соединение к серверу
 * ------------------------------------------------------------------------------------------- */
static void hndConnecting() {
    if (!cli.connect("gpstat.dev.cliffa.net", 9971)) {
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
