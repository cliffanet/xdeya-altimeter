
#include "jump.h"
#include "../altimeter.h"
#include "../gps/proc.h"
#include "../file/log.h"
#include "../file/track.h"
#include "../view/base.h"   // btnPushed()
#include "../power.h"       // pwrBattValue()

#include <TimeLib.h>

/* ------------------------------------------------------------------------------------------- *
 *  Пишем в простой лог прыжки
 * ------------------------------------------------------------------------------------------- */
static enum {
    JMP_NONE,
    JMP_FREEFALL,
    JMP_CANOPY
} jmpst = JMP_NONE;

ConfigJump jmp;

ConfigJump::ConfigJump() :
    Config(PSTR(CFG_JUMP_NAME), CFG_JUMP_VER)
{
    log_item_t li = { 0 };
    li.state = ACST_INIT;
    li.direct = ACDIR_INIT;
    tm_t tm = { 0 }; 
    
    data.last.num = 0;
    data.last.tm = tm;
    data.last.beg = li;
    data.last.cnp = li;
    data.last.end = li;
}

/* ------------------------------------------------------------------------------------------- *
 *  Старт прыга, инициируем новый прыжок
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::beg() {
    if (data.state != LOGJMP_NONE)
        return false;
    
    data.count++;
    data.state = LOGJMP_BEG;
    
    data.last.num = data.count;
    data.last.tm = tmNow();
    
    tmval = tmValue();
    
    auto li = jmpLogItem(tmval);
    data.last.beg = li;
    data.last.cnp = li;
    data.last.end = li;
    
    return save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Под куполом, сохраняем промежуточные значения
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::cnp() {
    if (data.state != LOGJMP_BEG)
        return false;
    
    data.state = LOGJMP_CNP;
    
    auto li = jmpLogItem(tmval);
    data.last.cnp = li;
    data.last.end = li;
    
    return save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Окончание прыга, пишем в логбук
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::end() {
    if (data.state == LOGJMP_NONE)
        return false;
    
    data.state = LOGJMP_NONE;
    
    data.last.end = jmpLogItem(tmval);
    
    if (!save(true))
        return false;
    
    struct log_item_s<log_jmp_t> jlast(data.last);
    if (!logAppend(PSTR(JMPLOG_SIMPLE_NAME), jlast, JMPLOG_SIMPLE_ITEM_COUNT, JMPLOG_SIMPLE_FILE_COUNT))
        return false;
    
    return true;
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
        .altspeed   = ac.speed()*100,
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
        case ACDIR_UP:          li.direct = 'u'; break;
        case ACDIR_NULL:        li.direct = 'n'; break;
        case ACDIR_DOWN:        li.direct = 'd'; break;
    }
    
    return li;
}

/* ------------------------------------------------------------------------------------------- *
 *  Определяем текущее состояние прыга и переключаем по необходимости
 * ------------------------------------------------------------------------------------------- */
void jmpProcess() {
    auto &ac = altCalc();
    
    if ((jmpst == JMP_NONE) &&
        ((ac.state() == ACST_FREEFALL) || (ac.state() == ACST_CANOPY)) &&
        (ac.direct() == ACDIR_DOWN) &&
        (ac.dircnt() > JMP_ALTI_DIRDOWN_COUNT)) {
        // Включаем запись лога прыга
        jmpst = JMP_FREEFALL; // Самое начало прыга помечаем в любом случае как FF,
                              // т.к. из него можно перейти в CNP, но не обратно (именно для jmp)
        
        if (!trkRunning())
            trkStart(false);
        
        jmp.beg();
    }
    
    if ((jmpst > JMP_FREEFALL) && (ac.state() == ACST_CANOPY)) {
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
        //if (jmp == JMP_FREEFALL) // Был пропущен момент перехода на JMP_CANOPY
        //    jmpcnp = li;
        jmpst = JMP_NONE;
        
        if (trkState() == TRKRUN_AUTO)
            trkStop();
        
        // Сохраняем
        jmp.end();
    }
}

void jmpReset() {
    jmpst = JMP_NONE;
    if (jmp.state() != LOGJMP_NONE)
        jmp.end();
}
