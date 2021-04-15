/*
    common
*/

#ifndef _def_H
#define _def_H

#include <Arduino.h>
#include <TimeLib.h>

#define HWVER       3

#if HWVER < 3
#define USE4BUTTON  1
#endif


// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   3600000
bool timeOk();

#endif // _def_H
