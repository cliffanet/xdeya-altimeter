
#include "proc.h"
#include "../log.h"
#include "../cfg/main.h"
#include "../gps/proc.h"
#include "../file/track.h"
#include "../view/base.h"   // btnPushed()
#include "../power.h"       // pwrBattRaw()
#include "../clock.h"

#include <Adafruit_BMP280.h>

#if HWVER <= 1
static Adafruit_BMP280 bmp; // hardware Wire
#else
static Adafruit_BMP280 bmp(5); // hardware SPI on IO5
#endif

static AltCalc ac;

static jmp_cur_t logcursor;
static log_item_t logall[JMP_PRELOG_SIZE] = { { 0 } };

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции
 * ------------------------------------------------------------------------------------------- */

AltCalc & altCalc() {
    return ac;
}

/* ------------------------------------------------------------------------------------------- *
 *  Сбор текущих данных
 * ------------------------------------------------------------------------------------------- */
static void jmpPreLogAdd(uint16_t interval) {
    auto &gps = gpsInf();
    
    logcursor ++;
    auto &li = logall[*logcursor];
    
    li = {
        tmoffset    : interval,
        flags       : 0,
        state       : 'U',
        direct      : 'U',
        alt         : ac.alt() + cfg.d().altcorrect,
        altspeed    : ac.speedapp()*100,
        lon         : gps.lon,
        lat         : gps.lat,
        hspeed      : gps.gSpeed,
        heading     : GPS_DEG(gps.heading),
        gpsalt      : GPS_MM(gps.hMSL),
        vspeed      : gps.speed,
        gpsdage     : gpsDataAge(), // тут вместо millis уже используется только 1 байт для хранения, можно пересмотреть формат
        sat         : gps.numSV,
        _           : 0,
#if HWVER > 1
        batval      : pwrBattRaw(),
#else
        batval      : 0,
#endif
        hAcc        : gps.hAcc,
        vAcc        : gps.vAcc,
        sAcc        : gps.sAcc,
        cAcc        : gps.cAcc,
        //tm          : gps.tm,
        millis      : utm() / 1000,
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
}

/* ------------------------------------------------------------------------------------------- *
 *  Получение текущего индекса и данных по любому индексу
 * ------------------------------------------------------------------------------------------- */
const jmp_cur_t &jmpPreCursor() {
    return logcursor;
}
const log_item_t &jmpPreLog(const jmp_cur_t &cursor) {
    return logall[*cursor];
}

uint32_t jmpPreInterval(const jmp_cur_t &from) {
    jmp_cur_t cursor = logcursor;
    uint32_t interval = 0;
    
    while (cursor != from) {
        interval += logall[*cursor].tmoffset;
        cursor--;
    }
    
    return interval;
}

/* ------------------------------------------------------------------------------------------- *
 *  Текущее давление "у земли", нужно для отслеживания начала подъёма в режиме "сон"
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR float _pressgnd = 101325, _altlast = 0;
static RTC_DATA_ATTR int8_t _toffcnt = 0;
bool jmpTakeoffCheck() {
    jmpInit();
    
    float press = bmp.readPressure();
    float alt = press2alt(_pressgnd, press);
    CONSOLE("_pressgnd: %.2f, press: %.2f, alt: %.2f, _altlast: %.2f, _toffcnt: %d", _pressgnd, press, alt, _altlast, _toffcnt);
    
    if (alt > 100) {
        ac.gndset(press);
        CONSOLE("alt: %0.0f", alt);
        return true;
    }
    if (alt > (_altlast + 0.5)) {
        if (_toffcnt < 0)
            _toffcnt = 0;
        _toffcnt ++;
        if (_toffcnt >= 10) {
            CONSOLE("is toff");
            return true;
        }
        else
            _toffcnt = 0;
    }
    else {
        if (_toffcnt > 0)
            _toffcnt --;
        _toffcnt --;
        if (_toffcnt <= -10) {
            CONSOLE("gnd reset");
            _pressgnd = press;
            _toffcnt = 0;
        }
    }
    
    _altlast = alt;
    
    return false;
}

/* ------------------------------------------------------------------------------------------- *
 *  Обработка изменения режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_jmpmode_t prev, ac_jmpmode_t jmpmode) {
    if ((prev == ACJMP_FREEFALL) && cfg.d().dsplautoff) {
        // Восстанавливаем обработчики после принудительного FF-режима
        setViewMain();
    }

    uint32_t jmpcnt = ac.jmpcnt();

    if ((prev <= ACJMP_TAKEOFF) && (jmpmode > ACJMP_TAKEOFF)) {
        // Начало прыга
        // Т.к. прыг по версии altcalc начинается не с самого момента отделения,
        // а с момента, когда вертикальная скорость превысила порог,
        // эта скорость наступает не сразу, а примерно через 3 сек после отделения,
        // Однако, это справедливо только для начала прыга, поэтому это значение прибавляем
        // только из перехода из статичного состояния (prev <= ACJMP_TAKEOFF)
        jmpcnt += 30;
        // для удобства отладки помечаем текущий jmpPreLog
        // флагом LI_FLAG_JMPDECISS - момент принятия решения о начале прыжка
        logall[*logcursor].flags            |= LI_FLAG_JMPDECISS;
        // А момент, который мы считаем отделением - флагом LI_FLAG_JMPBEG
        logall[*(logcursor - jmpcnt)].flags |= LI_FLAG_JMPBEG;

        // Временно сделаем, чтобы при автозапуске трека, он начинал писать чуть заранее до отделения
        trkStart(TRK_RUNBY_JMPBEG, jmpcnt+50);
    }
    
    switch (jmpmode) {
        case ACJMP_TAKEOFF:
            jmp.toff(ac.jmpcnt() + 50);
            break;
        
        case ACJMP_FREEFALL:
            if (cfg.d().dsplautoff)
                setViewMain(MODE_MAIN_ALT, false);
            
            jmp.beg(jmpcnt);
            break;
        
        case ACJMP_CANOPY:
            setViewMain(cfg.d().dsplcnp, false);
            
            logall[*(logcursor - jmpcnt)].flags  |= LI_FLAG_JMPCNP;
            jmp.cnp(jmpcnt);
            break;
            
        case ACJMP_NONE:
            setViewMain(cfg.d().dsplland);
            
            // Прыг закончился совсем, сохраняем результат
            logall[*logcursor].flags  |= LI_FLAG_JMPEND;
        
            // Сохраняем
            jmp.end();
            
            // Остановка трека, запущенного автоматически
            trkStop(TRK_RUNBY_JMPBEG | TRK_RUNBY_ALT);

            // на земле выключаем gps после включения на заданной высоте
            gpsOff(GPS_PWRBY_ALT);

            // Принудительное отключение gps после приземления
            if (cfg.d().gpsoffland && gpsPwr())
                gpsOff();
            
            break;
    }
    
    tmcntReset(TMCNT_NOFLY, jmpmode == ACJMP_NONE);
}

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void jmpInit() {
    if (!bmp.begin(BMP280_ADDRESS_ALT)) {   
        CONSOLE("Could not find a valid BMP280 sensor, check wiring!");
    }
    // В режиме sleep у нас постоянно мониторится "давление земли"
    // Используем его при выходе из сна, 
    // особенно, если выход из сна по причине начала подъёма
    if (pwrMode() == PWR_SLEEP)
        ac.gndset(_pressgnd);
}

/* ------------------------------------------------------------------------------------------- *
 *  Определяем текущее состояние прыга и переключаем по необходимости
 * ------------------------------------------------------------------------------------------- */
void jmpProcess() {
    static uint32_t tck;
    uint32_t interval = utm_diff32(tck, tck);
    ac.tick(bmp.readPressure(), interval / 1000);
    jmpPreLogAdd(interval / 1000);
    _pressgnd = ac.pressgnd();
    _altlast = ac.alt();
    _toffcnt = 0;
    
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
    static auto jmpmode = ac.jmpmode();
    if (jmpmode != ac.jmpmode())
        altState(jmpmode, ac.jmpmode());
    jmpmode = ac.jmpmode();
    
    if (ac.jmpmode() == ACJMP_TAKEOFF) {
        // автовключение gps на заданной высоте
        if ((cfg.d().gpsonalt > 0) &&
            (ac.alt() >= cfg.d().gpsonalt) &&
            !gpsPwr(GPS_PWRBY_ALT))
            gpsOn(GPS_PWRBY_ALT);
    
        // автовключение записи трека на заданной высоте
        if ((cfg.d().trkonalt > 0) &&
            (ac.alt() >= cfg.d().trkonalt) &&
            !trkRunning(TRK_RUNBY_ALT))
            trkStart(TRK_RUNBY_ALT);
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Принудительный сброс текущего прыга
 * ------------------------------------------------------------------------------------------- */
void jmpReset() {
    if (jmp.state() != LOGJMP_NONE)
        jmp.end();
}
