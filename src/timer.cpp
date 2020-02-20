
#include "timer.h"

static uint32_t btnHolded = 0;
static uint8_t btpPushed = 0;

static uint32_t tout = 0;
static timer_hnd_t thnd = NULL;

/* ------------------------------------------------------------------------------------------- *
 * События кнопок: нажатие, отпускание, удержание
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
