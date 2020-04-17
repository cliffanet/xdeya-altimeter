
#include "def.h"

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

bool is_on = true;

// Обработка процесса текущего режима
void (*hndProcess)() = NULL;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
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
        
    listDir(SPIFFS, "/", 0);

    switch (cfg.d().dsplpwron) {
        case MODE_MAIN_GPS:
        case MODE_MAIN_ALT:
        case MODE_MAIN_ALTGPS:
        case MODE_MAIN_TIME:
            if (inf.d().mainmode != cfg.d().dsplpwron)
                inf.set().mainmode = cfg.d().dsplpwron;
            break;
    }
    initMain(inf.d().mainmode);
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
        //Serial.printf("bef: %ld\r\n", gps.time.age());
        if ((gps.time.age() >= 0) && (gps.time.age() < 500)) {
            // set the Time to the latest GPS reading
            setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
            adjustTime(cfg.d().timezone * 60);
            tmadj = millis() + TIME_ADJUST_INTERVAL;
        }
        //Serial.printf("aft: %ld\r\n", gps.time.age());
    }

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
