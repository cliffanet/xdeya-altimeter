/*
    Altimeter
*/

#ifndef _altimeter_H
#define _altimeter_H

#include <Arduino.h>
#include "altcalc.h"

// Шаг отображения высоты
#define ALT_STEP            5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND      3

// Интервал обнуления высоты (в количествах тиков)
#define ALT_AUTOGND_INTERVAL    6000

typedef void (*altimeter_state_hnd_t)(ac_state_t prev, ac_state_t state);

altcalc & altCalc();
void altInit();
void altProcess();
void altStateHnd(altimeter_state_hnd_t hnd);

#endif // _altimeter_H
