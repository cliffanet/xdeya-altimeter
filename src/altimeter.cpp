
#include "altimeter.h"
#include "eeprom.h"

#include <Adafruit_BMP280.h>

bool bmpok = false;
static Adafruit_BMP280 bme; // hardware Wire
static altcalc ac;
static altimeter_state_hnd_t statehnd = NULL;

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции обновления дисплея
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
    ac_state_t st = ac.state();
    ac.tick(bme.readPressure());
    
    // Автокорректировка нуля
    if (cfg.gndauto &&
        (ac.state() == ACST_GROUND) &&
        (ac.direct() == ACDIR_NULL) &&
        (ac.dircnt() >= ALT_AUTOGND_INTERVAL)) {
        ac.gndreset();
        Serial.println("auto GND reseted");
    }
    
    // Обработка изменения режима высотомера
    if ((statehnd != NULL) && (st != ac.state()))
        statehnd(st, ac.state());
}


void altStateHnd(altimeter_state_hnd_t hnd) {
    statehnd = hnd;
}
