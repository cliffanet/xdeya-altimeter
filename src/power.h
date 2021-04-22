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
#if HWVER >= 3
#define HWPOWER_PIN_BATCHRG 12
#endif

bool pwrCheck();

void pwrOff();

#if HWVER > 1
uint16_t pwrBattValue();
#endif
#if HWVER >= 3
bool pwrBattCharge();
#endif

#endif // _power_H
