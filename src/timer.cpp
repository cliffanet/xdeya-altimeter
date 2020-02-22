
#include "timer.h"

static uint32_t tout = 0;
static timer_hnd_t thnd = NULL;

/* ------------------------------------------------------------------------------------------- *
 * Действие по таймеру, привязано к millis()
 * ------------------------------------------------------------------------------------------- */

void timerHnd(timer_hnd_t hnd, uint32_t timeout) {
    thnd = hnd;
    tout = millis() + timeout;
}

void timerUpdate(uint32_t timeout) {
    tout = millis() + timeout;
}

void timerClear() {
    thnd = NULL;
    tout = 0;
}

void timerProcess() {
    if (thnd == NULL)
        return;
    if (tout > millis())
        return;
    thnd();
    timerClear();
}
