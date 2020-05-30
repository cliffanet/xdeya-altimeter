/*
    Menu Base class
*/

#ifndef _power_H
#define _power_H

#include "../def.h"

#if HWVER > 1
#define HWPOWER_PIN_GPS     27
#define HWPOWER_PIN_BATIN   36
#endif

bool pwrCheck();

void pwrOff();

#if HWVER > 1
uint16_t pwrBattValue();
#endif

#endif // _power_H
