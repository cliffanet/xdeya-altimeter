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
}

/* ------------------------------------------------------------------------------------------- *
 *  чтение инфы от сервера, возвращает true, если есть инфа
 * ------------------------------------------------------------------------------------------- */
static bool srvWaitHdr() {
    if (cli.available() <= 0) {
        if (!cli.connected())
            ERR("server connect lost");
        return cli.connected();
    }
    
    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Ожидание ответа на приветствие
 * ------------------------------------------------------------------------------------------- */
static void waitHello() {
    if (!srvWaitHdr())
        return;
    Serial.printf("server avail: %d\r\n", cli.available());
}

/* ------------------------------------------------------------------------------------------- *
 *  Соединение к серверу
 * ------------------------------------------------------------------------------------------- */
static void hndConnecting() {
    if (!cli.connect("gpstat.dev.cliffa.net", 9971)) {
        ERR("server can't connect");
        return;
    }
    Serial.println("server connected");
    
    ConfigWebJoin wjoin;
    if (!wjoin.load()) {
        ERR("Can\'t load JOIN-inf");
        return;
    }
    
    uint8_t d[8] = { '%', 0x01, 0, 4 };
    uint32_t id = htonl(wjoin.extid());
    memcpy(d+4, &id, sizeof(id));
    
    cli.write(d, sizeof(d));
    
    hndProcess = waitHello;
}

/* ------------------------------------------------------------------------------------------- *
 *  Ожидание соединение по wifi
 * ------------------------------------------------------------------------------------------- */
static void waitWiFiConnect() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("wifi connected");
        strcpy_P(title, PSTR("connecting to server"));
        displayUpdate();
        hndProcess = hndConnecting;
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
    
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char *)ssid, "12344321");
}
