
#include "proc.h"
#include "../log.h"
#include "../cfg/main.h"
#include "../gps/proc.h"
#include "../file/track.h"
#include "../view/base.h"   // btnPushed()
#include "../power.h"       // pwrBattValue()

#include <Adafruit_BMP280.h>

#if HWVER <= 1
static Adafruit_BMP280 bmp; // hardware Wire
#else
static Adafruit_BMP280 bmp(5); // hardware SPI on IO5
#endif

static AltCalc ac;

static enum {
    JMP_NONE,
    JMP_FREEFALL,
    JMP_CANOPY
} jmpst = JMP_NONE;

/* ------------------------------------------------------------------------------------------- *
 *  Обработка изменения режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_state_t prev, ac_state_t curr) {
    if ((prev == ACST_FREEFALL) && (curr != ACST_FREEFALL) && cfg.d().dsplautoff) {
        // Восстанавливаем обработчики после принудительного FF-режима
        setViewMain();
    }
    
    switch (curr) {
        case ACST_GROUND:
            setViewMain(cfg.d().dsplland);
            break;
        
        case ACST_FREEFALL:
            if (cfg.d().dsplautoff)
                setViewMain(MODE_MAIN_ALT, false);
            break;
        
        case ACST_CANOPY:
            setViewMain(cfg.d().dsplcnp, false);
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции
 * ------------------------------------------------------------------------------------------- */

AltCalc & altCalc() {
    return ac;
}

/* ------------------------------------------------------------------------------------------- *
 *  Сбор текущих данных
 * ------------------------------------------------------------------------------------------- */
log_item_t jmpLogItem(const tm_val_t &tmval) {
    auto &ac = altCalc();
    auto &gps = gpsInf();
    
    log_item_t li = {
        .tmoffset   = tmInterval(tmval),
        .flags      = 0,
        .state      = 'U',
        .direct     = 'U',
        .alt        = ac.alt(),
        .altspeed   = ac.speedapp()*100,
        .lon        = gps.lon,
        .lat        = gps.lat,
        .hspeed     = gps.gSpeed,
        .heading    = GPS_DEG(gps.heading),
        .gpsalt     = GPS_MM(gps.hMSL),
        .vspeed     = gps.speed,
        .gpsdage    = gpsDataAge(),
        .sat        = gps.numSV,
        ._          = 0,
#if HWVER > 1
        .batval     = pwrBattValue(),
#else
        .batval     = 0,
#endif
        .hAcc       = gps.hAcc,
        .vAcc       = gps.vAcc,
        .sAcc       = gps.sAcc,
        .cAcc       = gps.cAcc,
        .tm         = gps.tm,
    };
    
    if (GPS_VALID(gps))
        li.flags |= LI_FLAG_GPS_VALID;
    if (GPS_VALID_LOCATION(gps))
        li.flags |= LI_FLAG_GPS_VLOC;
    if (GPS_VALID_VERTICAL(gps))
        li.flags |= LI_FLAG_GPS_VVERT;
    if (GPS_VALID_SPEED(gps))
        li.flags |= LI_FLAG_GPS_VSPEED;
    if (GPS_VALID_HEAD(gps))
        li.flags |= LI_FLAG_GPS_VHEAD;
    if (GPS_VALID_TIME(gps))
        li.flags |= LI_FLAG_GPS_VTIME;
    
    if (btnPushed(BTN_UP))
        li.flags |= LI_FLAG_BTN_UP;
    if (btnPushed(BTN_SEL))
        li.flags |= LI_FLAG_BTN_SEL;
    if (btnPushed(BTN_DOWN))
        li.flags |= LI_FLAG_BTN_DOWN;
    
    switch (ac.state()) {
        case ACST_INIT:         li.state = 'i'; break;
        case ACST_GROUND:       li.state = 'g'; break;
        case ACST_TAKEOFF40:    li.state = 's'; break;
        case ACST_TAKEOFF:      li.state = 't'; break;
        case ACST_FREEFALL:     li.state = 'f'; break;
        case ACST_CANOPY:       li.state = 'c'; break;
        case ACST_LANDING:      li.state = 'l'; break;
    }
    switch (ac.direct()) {
        case ACDIR_INIT:        li.direct = 'i'; break;
        case ACDIR_ERR:         li.direct = 'e'; break;
        case ACDIR_UP:          li.direct = 'u'; break;
        case ACDIR_FLAT:        li.direct = 'f'; break;
        case ACDIR_DOWN:        li.direct = 'd'; break;
    }
    
    return li;
}

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void jmpInit() {
    if (!bmp.begin(BMP280_ADDRESS_ALT)) {   
        CONSOLE("Could not find a valid BMP280 sensor, check wiring!");
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Определяем текущее состояние прыга и переключаем по необходимости
 * ------------------------------------------------------------------------------------------- */
void jmpProcess() {
    static uint32_t _mill = millis();
    uint32_t m = millis();
    ac.tick(bmp.readPressure(), m-_mill);
    _mill = m;
    
    // Автокорректировка нуля
    if (cfg.d().gndauto &&
        (ac.state() == ACST_GROUND) &&
        (ac.direct() == ACDIR_FLAT) &&
        (ac.dirtm() >= ALT_AUTOGND_INTERVAL)) {
        ac.gndreset();
        ac.dirreset();
        CONSOLE("auto GND reseted");
    }
    
    // Обработка изменения режима высотомера
    static auto altstate = ac.state();
    if (altstate != ac.state())
        altState(altstate, ac.state());
    altstate = ac.state();
    
    if ((jmpst == JMP_NONE) &&
        ((ac.state() == ACST_FREEFALL) || (ac.state() == ACST_CANOPY))) {
        // Включаем запись лога прыга
        jmpst = JMP_FREEFALL; // Самое начало прыга помечаем в любом случае как FF,
                              // т.к. из него можно перейти в CNP, но не обратно (именно для jmp)
        
        if (!trkRunning())
            trkStart(false);
        
        jmp.beg();
    }
    
    if ((jmpst == JMP_FREEFALL) && (ac.state() == ACST_CANOPY)) {
        // Переход в режим CNP после начала прыга,
        // Дальше только окончание прыга может быть, даже если начнётся снова FF,
        // Для jmp только такой порядок переходов,
        // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
        jmpst = JMP_CANOPY;
        
        // Сохраняем промежуточный результат
        jmp.cnp();
    }
    
    if ((jmpst > JMP_NONE) && (ac.state() == ACST_GROUND)) {
        // Прыг закончился совсем, сохраняем результат
        jmpst = JMP_NONE;
        
        if (trkState() == TRKRUN_AUTO)
            trkStop();
        
        // Сохраняем
        jmp.end();
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Принудительный сброс текущего прыга
 * ------------------------------------------------------------------------------------------- */
void jmpReset() {
    jmpst = JMP_NONE;
    if (jmp.state() != LOGJMP_NONE)
        jmp.end();
}
