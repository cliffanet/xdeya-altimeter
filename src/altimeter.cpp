
#include "altimeter.h"

#include <Adafruit_BMP280.h>

bool bmpok = false;
static Adafruit_BMP280 bme; // hardware Wire
static altcalc ac;

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
    ac.tick(bme.readPressure());
}
