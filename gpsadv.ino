
#include "def.h"

#include "src/log.h"
#include "src/power.h"
#include "src/clock.h"
#include "src/view/main.h"
#include "src/gps/proc.h"
#include "src/jump/proc.h"
#include "src/file/track.h"
#include "src/cfg/main.h"
#include "src/cfg/point.h"
#include "src/cfg/jump.h"

#include <WiFi.h>
#include <SPIFFS.h>

//------------------------------------------------------------------------------
void setup() {

#ifdef FWVER_DEBUG
    Serial.begin(115200);
#endif // FWVER_DEBUG
    
    if (!pwrCheck())
        return;
    
    CONSOLE("Firmware " FWVER_FILENAME "; Build Date: " __DATE__);
  
    WiFi.mode(WIFI_OFF);

    // часы
    clockInit();

    // инициируем view
    viewInit();
    
    // инициализируем высотомер
    jmpInit();

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
    uint32_t m1 = millis();
    clockProcess();
    gpsProcess();
    jmpProcess();
    trkProcess();
    viewProcess();
    
    uint32_t m2 = millis();
    uint32_t md = m2-m1;
    if (md < 100)
        delay(100-md);
        
    jmpProcess();
    
    md = millis() - m2;
    if (md < 100)
        delay(100-md);
}
