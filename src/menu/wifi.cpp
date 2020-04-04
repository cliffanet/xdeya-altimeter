
#include "wifi.h"
#include "../net/netsync.h"

#include <Arduino.h>
#include <WiFi.h>

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса MenuWifi
 * ------------------------------------------------------------------------------------------- */

static uint16_t wifiInit() {
    Serial.println("Setup begin");
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup done");
    int n = WiFi.scanNetworks();
    Serial.print(F("scan: "));
    Serial.println(n);
    return n;
}

static bool wifiConnect(const char *net) {
    Serial.printf("wifi to: %s\r\n", net);
    
    WiFi.begin(net, "12344321");
    
    uint32_t m = millis() + 10000;
    while (m > millis()) {
        auto st = WiFi.status();
        if (st != WL_CONNECTED) continue;
        break;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("wifi can't connect");
        WiFi.mode(WIFI_OFF);
        return false;
    }
    
    Serial.println("wifi connected");
    
    WiFiClient cli;
    NetSync ns(cli);
    while (ns.process())
        delay(100);
    
    if (ns.error())
        Serial.printf("Sync error: %s\r\n", ns.errstr());
    
    WiFi.mode(WIFI_OFF);
    
    Serial.println("finish");
    return true;
}

MenuWiFi::MenuWiFi() :
    MenuBase(wifiInit(), PSTR("WiFi Sync"))
{
    wifiall.clear();
    for (int i = 0; i < size(); ++i) {
        wifi_t w;
        strncpy(w.name, WiFi.SSID(i).c_str(), sizeof(w.name));
        w.name[sizeof(w.name)-1] = '\0';
        w.isopen = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
        snprintf_P(w.txt, sizeof(w.txt), PSTR("%s (%d) %c"), w.name, WiFi.RSSI(i), w.isopen?' ':'*');
        wifiall.push_back(w);
    }
    
    Serial.printf("scan end (%d)\r\n", wifiall.size());
    
    for(auto const &w : wifiall)
        Serial.println(w.txt);
    Serial.printf("wifiall: %d\r\n", wifiall.size());
}

MenuWiFi::~MenuWiFi() {
    WiFi.mode(WIFI_OFF);
    Serial.println("wifi end");
    
}

void MenuWiFi::updStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuWiFi::updStr: %d (sz=%d)\r\n", i, wifiall.size());
    auto const &w = wifiall[i];
    strncpy(str.name, w.txt, sizeof(str.name));
    str.name[sizeof(str.name)-1] = '\0';
    str.val[0] = w.isopen ? '\0' : '*';
    str.val[1] = '\0';
}

void MenuWiFi::btnSmp() {
    auto const &w = wifiall[sel()];
    wifiConnect(w.name);
}
