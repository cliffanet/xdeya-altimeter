
#include "wifi.h"
#include "../mode.h"
#include "../file/wifi.h"

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

void MenuWiFi::getStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuWiFi::getStr: %d (sz=%d)\r\n", i, wifiall.size());
    auto const &w = wifiall[i];
    strncpy(str.name, w.txt, sizeof(str.name));
    str.name[sizeof(str.name)-1] = '\0';
    if (w.isopen) {
        str.val[0] = '\0';
    }
    else {
        str.val[0] = wifiPassFind(w.name) ? '+' : 'x';
    }
    str.val[1] = '\0';
}

void MenuWiFi::btnSmp() {
    auto const &w = wifiall[sel()];
    if (w.isopen)
        modeNetSync(w.name);
    else {
        char pass[64];
        if (!wifiPassFind(w.name, pass)) {
            menuFlashP(PSTR("Password required!"));
            return;
        }
        modeNetSync(w.name, pass);
    }
}
