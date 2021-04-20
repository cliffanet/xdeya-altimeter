
#include "def.h"

#include "src/log.h"
#include "src/power.h"
#include "src/view/main.h"
#include "src/gps.h"
#include "src/altcalc.h"
#include "src/altimeter.h"
#include "src/file/track.h"
#include "src/cfg/main.h"
#include "src/cfg/point.h"
#include "src/cfg/jump.h"

#include <WiFi.h>
#include <SPIFFS.h>

#include <TimeLib.h>
static uint32_t tmadj = 0;
bool timeOk() { return (tmadj > 0) && ((tmadj > millis()) || ((millis()-tmadj) >= TIME_ADJUST_TIMEOUT)); }

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    if (!pwrCheck())
        return;
  
    WiFi.mode(WIFI_OFF);

    // инициируем view
    viewInit();
    
    // инициализируем высотомер
    altInit();

    // инициируем gps
    gpsInit();

    // загружаем сохранённый конфиг
    if(!SPIFFS.begin(true))
        CONSOLE("SPIFFS Mount Failed");
    cfgLoad(true);
    CONSOLE("begin");
}

//------------------------------------------------------------------------------



uint16_t _testDeg = 0;
uint16_t testDeg() { return _testDeg; }

//------------------------------------------------------------------------------

void loop() {
    gpsProcess();

    if (millis() >= tmadj) {
        auto &gps = gpsInf();
        //CONSOLE("bef: %ld", gps.time.age());
        if (GPS_TIME_VALID(gps)) {
            // set the Time to the latest GPS reading
            setTime(gps.tm.h, gps.tm.m, gps.tm.s, gps.tm.day, gps.tm.mon, gps.tm.year);
            adjustTime(cfg.d().timezone * 60);
            tmadj = millis() + TIME_ADJUST_INTERVAL;
        }
        //CONSOLE("aft: %ld", gps.time.age());
    }

/*
    if (Serial.available()) {
        char ch = Serial.read();
        switch (ch) {
            case '1':
                btnPush(BTN_UP, BTN_SIMPLE);
                break;
            case '2':
                btnPush(BTN_SEL, BTN_SIMPLE);
                break;
            case '3':
                btnPush(BTN_DOWN, BTN_SIMPLE);
                break;
            case '4':
                btnPush(BTN_UP, BTN_LONG);
                break;
            case '5':
                btnPush(BTN_SEL, BTN_LONG);
                break;
            case '6':
                btnPush(BTN_DOWN, BTN_LONG);
                break;
        }
    }
    */

    altProcess();
    jmpProcess();
    
    viewProcess();
    
    delay(100);
    altProcess();
    viewProcess();
    trkProcess();
    delay(100);
}
