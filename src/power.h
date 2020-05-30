/*
    Menu Base class
*/

#ifndef _power_H
#define _power_H

#include "../def.h"

#if HWVER > 1
#define HWPOWER_PIN_GPS 27
#endif


bool pwrCheck();

void pwrOff();

#endif // _power_H
