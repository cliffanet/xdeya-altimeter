/*
    WiFi functions
*/

#ifndef _net_wifi_H
#define _net_wifi_H

#include "../../def.h"
#include <stdint.h>

/*
 *  Использование WiFi.h не позволяет включить модуль так,
 *  чтобы сразу же понизить мощность передатчика.
 *
 *  Изменить можность передатчика можно только после инициализации wifi,
 *  но сделать это надо ещё до старта модуля, т.к. если старт происходит
 *  с повышенной мощностью, просадка напряжения заставляет сработать watchdog
 *  по питанию у самой esp23.
 *  
 *  Старт wifi назодится сразу после инициализации в WiFi.mode() и вклиниться в эту
 *  процедуру стардартными способами WiFi.h никак не получается.
 *
 *  Поэтому вместо использования WiFi.h придётся работать напрямую с esp32-sdk
 */

typedef struct {
    char ssid[33];
    int8_t rssi;
    bool isopen;
} wifi_net_t;

bool wifiStart();
bool wifiStop();

uint16_t wifiScan(bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0);
const wifi_net_t * wifiScanInfo(uint16_t i);

int8_t wifiPower();

#endif // _net_wifi_H
