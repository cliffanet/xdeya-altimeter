/*
    Power functions
*/

#ifndef _power_H
#define _power_H

#include "../def.h"

#include <stdint.h>

#if HWVER > 1
#define HWPOWER_PIN_BATIN   36
#endif
#if HWVER >= 3
#define HWPOWER_PIN_BATCHRG 12
#endif

#define BATT_VALUE_MIN      3.15

typedef enum {
    PWR_OFF = 0,
    PWR_SLEEP,
    PWR_PASSIVE,
    PWR_ACTIVE
} power_mode_t;

#define PWR_SLEEP_TIMEOUT   30000

power_mode_t pwrMode();
bool pwrInit();
void pwrModeUpd();
void pwrRun(void (*run)());

void pwrSleep();
void pwrOff();

#if HWVER > 1
bool pwrBattChk(bool init = false, double val = BATT_VALUE_MIN);
uint16_t pwrBattRaw();
double pwrBattValue();
#else
#define pwrBattChk(...)
#endif
#if HWVER >= 3
bool pwrBattCharge();
#endif

#endif // _power_H
