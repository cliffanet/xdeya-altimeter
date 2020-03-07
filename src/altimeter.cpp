
#include "altimeter.h"
#include "cfg/main.h"

#include <Adafruit_BMP280.h>

bool bmpok = false;
static Adafruit_BMP280 bme; // hardware Wire
static altcalc ac;
static altimeter_state_hnd_t statehnd = NULL;

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции
 * ------------------------------------------------------------------------------------------- */

altcalc & altCalc() {
    return ac;
}

void altInit() {
    if (!bme.begin(BMP280_ADDRESS_ALT)) {   
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
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
        Serial.println("auto GND reseted");
    }
    
    // Обработка изменения режима высотомера
    if ((statehnd != NULL) && (ac.stateprev() != ac.state()))
        statehnd(ac.stateprev(), ac.state());
}


void altStateHnd(altimeter_state_hnd_t hnd) {
    statehnd = hnd;
}
