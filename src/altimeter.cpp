
#include "altimeter.h"
#include "log.h"
#include "cfg/main.h"
#include "../def.h"

#include <Adafruit_BMP280.h>

bool bmpok = false;

#if HWVER <= 1
static Adafruit_BMP280 bme; // hardware Wire
#else
static Adafruit_BMP280 bme(5); // hardware SPI on IO5
#endif

static altcalc ac;

/* ------------------------------------------------------------------------------------------- *
 *  Обработка изменения режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_state_t prev, ac_state_t curr) {
    if ((prev == ACST_FREEFALL) && (curr != ACST_FREEFALL) && cfg.d().dsplautoff) {
        // Восстанавливаем обработчики после принудительного FF-режима
        setViewMain();
    }
    
    switch (curr) {
        case ACST_GROUND:
            setViewMain(cfg.d().dsplland);
            break;
        
        case ACST_FREEFALL:
            if (cfg.d().dsplautoff)
                setViewMain(MODE_MAIN_ALT, false);
            break;
        
        case ACST_CANOPY:
            setViewMain(cfg.d().dsplcnp, false);
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции
 * ------------------------------------------------------------------------------------------- */

altcalc & altCalc() {
    return ac;
}

void altInit() {
    if (!bme.begin(BMP280_ADDRESS_ALT)) {   
        CONSOLE("Could not find a valid BMP280 sensor, check wiring!");
    }
}

void altProcess() {
    ac.tick(bme.readPressure());
    
    // Автокорректировка нуля
    if (cfg.d().gndauto &&
        (ac.state() == ACST_GROUND) &&
        (ac.direct() == ACDIR_NULL) &&
        (ac.dircnt() >= ALT_AUTOGND_INTERVAL)) {
        ac.gndreset();
        CONSOLE("auto GND reseted");
    }
    
    // Обработка изменения режима высотомера
    if (ac.stateprev() != ac.state())
        altState(ac.stateprev(), ac.state());
}
