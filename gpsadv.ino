
#include "def.h"

#include "src/power.h"
#include "src/button.h"
#include "src/display.h"
#include "src/mode.h"
#include "src/menu/base.h"
#include "src/timer.h"
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

// Центральный цикл, иногда надо и его подменить
void (*loopMain)() = NULL;

// Обработка процесса текущего режима
void (*hndProcess)() = NULL;

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    if (!pwrCheck())
        return;
  
    WiFi.mode(WIFI_OFF);
    
    displayInit();

    // инициируем кнопки
    btnInit();
    
    // инициализируем высотомер
    altInit();

    // инициируем gps
    gpsInit();

    // загружаем сохранённый конфиг
    if(!SPIFFS.begin(true))
        Serial.println("SPIFFS Mount Failed");
    cfgLoad(true);
    Serial.println("begin");

    switch (cfg.d().dsplpwron) {
        case MODE_MAIN_GPS:
        case MODE_MAIN_ALT:
        case MODE_MAIN_ALTGPS:
        case MODE_MAIN_TIME:
            setModeMain(cfg.d().dsplpwron);
            break;
    }
    modeMain();
}

//------------------------------------------------------------------------------



uint16_t _testDeg = 0;
uint16_t testDeg() { return _testDeg; }

//------------------------------------------------------------------------------
static void loopDefault() {
    gpsProcess();

    if (millis() >= tmadj) {
        auto &gps = gpsGet();
        //Serial.printf("bef: %ld\r\n", gps.time.age());
        if ((gps.time.age() >= 0) && (gps.time.age() < 500)) {
            // set the Time to the latest GPS reading
            setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
            adjustTime(cfg.d().timezone * 60);
            tmadj = millis() + TIME_ADJUST_INTERVAL;
        }
        //Serial.printf("aft: %ld\r\n", gps.time.age());
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
    
    btnProcess();
    if (hndProcess != NULL)
        hndProcess();
    timerProcess();
    displayUpdate();
    
    delay(100);
    altProcess();
    btnProcess();
    if (hndProcess != NULL)
        hndProcess();
    timerProcess();
    trkProcess();
    delay(100);
}

void loop() {
    if (loopMain == NULL)
        loopDefault();
    else
        loopMain();
}
