/*
    GPS
*/

#ifndef _gps_H
#define _gps_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include "eeprom.h"

// Количество сохраняемых GPS-точек
#define PNT_COUNT     3
#define PNT_NUMOK     ((cfg.pntnum > 0) && (cfg.pntnum <= PNT_COUNT))
#define POINT         cfg.pntall[cfg.pntnum-1]

TinyGPSPlus &gpsGet();
void gpsInit();
void gpsProcess();

#endif // _gps_H
