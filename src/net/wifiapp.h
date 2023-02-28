/*
    WiFi for application functions
*/

#ifndef _net_wifiapp_H
#define _net_wifiapp_H

#include "../../def.h"
#include <stdint.h>
#include <stddef.h>

void wifiCliBegin(const char *ssid, const char *pass = NULL);
bool wifiCliNet(char *ssid);
bool wifiCliIsRun();
bool wifiCliStop();

#endif // _net_wifiapp_H
