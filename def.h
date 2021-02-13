/*
    common
*/

#ifndef _def_H
#define _def_H

#include <Arduino.h>
#include <TimeLib.h>

#define HWVER       2
#define USE4BUTTON  1


// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   3600000
bool timeOk();

#endif // _def_H
