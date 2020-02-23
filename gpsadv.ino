
#include "def.h"

#include "src/button.h"
#include "src/display.h"
#include "src/mode.h"
#include "src/timer.h"
#include "src/eeprom.h"
#include "src/gps.h"
#include "src/altcalc.h"
#include "src/altimeter.h"

#include <TimeLib.h>
static uint32_t tmadj = 0;
bool timeOk() { return (tmadj > 0) && ((tmadj > millis()) || ((millis()-tmadj) >= TIME_ADJUST_TIMEOUT)); }

bool is_on = true;

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    infLoad();
    
    displayInit();

    // инициируем кнопки
    btnInit();
    
    // инициализируем высотомер
    altInit();

    // инициируем gps
    gpsInit();

    // загружаем сохранённый конфиг
    cfgLoad();
    cfgApply();
    Serial.println("begin");
    Serial.println(sizeof(cfg));

    switch (cfg.dsplpwron) {
        case MODE_MAIN_GPS:
        case MODE_MAIN_ALT:
        case MODE_MAIN_ALTGPS:
        case MODE_MAIN_TIME:
            inf.mainmode = cfg.dsplpwron;
            break;
    }
    initMain(inf.mainmode);
}

//------------------------------------------------------------------------------
void pwrOn() {
    if (is_on) return;
    is_on = true;
    displayOn();
        Serial.println("pwr on");
}

void pwrOff() {
    if (!is_on) return;
    is_on = false;
    displayOff();
        Serial.println("pwr off");
}

//------------------------------------------------------------------------------



uint16_t _testDeg = 0;
uint16_t testDeg() { return _testDeg; }

//------------------------------------------------------------------------------
void loop() {
    if (!is_on) {
        delay(1000); // Использование прерываний позволяет делать более длинный паузы в состоянии Off
        return;
    }

    gpsProcess();

    if (millis() >= tmadj) {
        auto &gps = gpsGet();
        if (gps.time.age() < 500) {
            // set the Time to the latest GPS reading
            setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
            adjustTime(cfg.timezone * 60);
            tmadj = millis() + TIME_ADJUST_INTERVAL;
        }
    }

    altProcess();
    btnProcess();
    timerProcess();
    displayUpdate();
    
    delay(100);
    altProcess();
    btnProcess();
    timerProcess();
    delay(100);
}
