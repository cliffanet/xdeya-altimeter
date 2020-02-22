/*
    common
*/

#ifndef _def_H
#define _def_H

#include <Arduino.h>
#include <TimeLib.h>


// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   3600000
bool timeOk();

// Логическое состояние включенности устройства
extern bool is_on;


void pwrOn();
void pwrOff();

#endif // _def_H
