
#include "def.h"

#include "src/log.h"
#include "src/power.h"
#include "src/clock.h"
#include "src/view/main.h"
#include "src/gps/proc.h"
#include "src/gps/compass.h"
#include "src/jump/proc.h"
#include "src/file/track.h"
#include "src/cfg/main.h"
#include "src/cfg/point.h"
#include "src/cfg/jump.h"

#include <SPIFFS.h>

//------------------------------------------------------------------------------
void setup() {

#ifdef FWVER_DEBUG
    Serial.begin(115200);
#endif // FWVER_DEBUG
    
    if (!pwrInit())
        return;
    
    CONSOLE("Firmware " FWVER_FILENAME "; Build Date: " __DATE__);
    
    if (pwrMode() != PWR_SLEEP) {
        tmcntReset(TMCNT_UPTIME, true);
        tmcntReset(TMCNT_NOFLY, true);
    }

    // инициируем view
    viewInit();
    
    // Инициализация сервисных пинов
#if HWVER >= 3
    pinMode(HWPOWER_PIN_BATCHRG, INPUT_PULLUP);
#endif

    // часы
    clockInit();
    
    // инициализируем высотомер
    jmpInit();

    // инициируем gps
    gpsRestore();

    // загружаем сохранённый конфиг
    if(!SPIFFS.begin(true))
        CONSOLE("SPIFFS Mount Failed");
    
    // Загружаем конфиги, но apply делаем только при холодном старте (не из sleep)    
    cfgLoad(pwrMode() != PWR_SLEEP);
    
    compInit();
    
    CONSOLE("begin");
}

//------------------------------------------------------------------------------

void loop() {
    pwrRun([]() {
        pwrBattChk();
        clockProcess();
        tmcntUpdate();
        gpsProcess();
        compProcess();
        jmpProcess();
        trkProcess();
        viewProcess();
    });
    pwrRun([]() {
        clockProcess();
        jmpProcess();
    });
}
