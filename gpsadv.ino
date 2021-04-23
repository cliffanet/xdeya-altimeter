
#include "def.h"

#include "src/log.h"
#include "src/power.h"
#include "src/clock.h"
#include "src/view/main.h"
#include "src/gps/proc.h"
#include "src/altcalc.h"
#include "src/altimeter.h"
#include "src/file/track.h"
#include "src/cfg/main.h"
#include "src/cfg/point.h"
#include "src/cfg/jump.h"

#include <WiFi.h>
#include <SPIFFS.h>

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    if (!pwrCheck())
        return;
  
    WiFi.mode(WIFI_OFF);

    // часы
    clockInit();

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

void loop() {
    clockProcess();
    gpsProcess();
    altProcess();
    jmpProcess();
    trkProcess();
    viewProcess();
    delay(100);
    altProcess();
    delay(100);
}
