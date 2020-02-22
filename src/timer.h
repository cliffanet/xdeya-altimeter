/*
    Timer
*/

#ifndef _timer_H
#define _timer_H

#include <Arduino.h>

typedef void (*timer_hnd_t)();

void timerHnd(timer_hnd_t hnd, uint32_t timeout);
void timerUpdate(uint32_t timeout);
void timerClear();
void timerProcess();

#endif // _timer_H
