/*
    Altimeter
*/

#ifndef _altimeter_H
#define _altimeter_H

#include "../def.h"
#include "altcalc.h"

// Шаг отображения высоты
#define ALT_STEP            5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND      3

altcalc & altCalc();
void altInit();
void altProcess();

#endif // _altimeter_H
