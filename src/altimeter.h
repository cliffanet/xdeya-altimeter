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

altcalc & altCalc();
void altInit();
void altProcess();

#endif // _altimeter_H
