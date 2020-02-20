/*
    common
*/

#ifndef _def_H
#define _def_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <TimeLib.h>


// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   3600000
bool timeOk();

// Логическое состояние включенности устройства
extern bool is_on;

// Количество сохраняемых GPS-точек
#define PNT_COUNT     3
#define PNT_NUMOK     ((cfg.pntnum > 0) && (cfg.pntnum <= PNT_COUNT))
#define POINT         cfg.pntall[cfg.pntnum-1]

#include "src/button.h"
#include "src/display.h"
#include "src/mode.h"
#include "src/timer.h"
#include "src/eeprom.h"


// GPS
TinyGPSPlus &gpsGet();

void pwrOn();
void pwrOff();

#endif // _def_H
