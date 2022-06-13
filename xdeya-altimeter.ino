
#include "def.h"

#include "src/log.h"
#include "src/power.h"
#include "src/clock.h"
#include "src/core/file.h"
#include "src/view/main.h"
#include "src/navi/proc.h"
#include "src/navi/compass.h"
#include "src/jump/proc.h"
#include "src/jump/track.h"
#include "src/cfg/main.h"
#include "src/cfg/point.h"
#include "src/cfg/jump.h"

#include <Wire.h>

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
    
    Wire.begin();

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

    //
    fileIntInit();
    
    // Загружаем конфиги, но apply делаем только при холодном старте (не из sleep)    
    cfgLoad(pwrMode() != PWR_SLEEP);
    
    if ((cfg.d().flagen & FLAGEN_COMPAS) > 0)
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
        viewProcess();
    });
    pwrRun([]() {
        
#ifdef CLOCK_EXTERNAL
        clockProcess();
#endif
        
        jmpProcess();
    });
}
