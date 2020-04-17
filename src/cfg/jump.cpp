
#include "jump.h"
#include "../altimeter.h"
#include "../gps.h"
#include "../file/log.h"
#include "../file/track.h"

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
    log_item_t li = {
        .mill   = 0,
        .alt    = 0,
        .vspeed = 0,
        .state  = ACST_INIT,
        .direct = ACDIR_INIT,
        .lat    = 0,
        .lng    = 0,
        .hspeed = 0,
        .hang   = 0,
        .sat    = 0,
    };
    dt_t dt = { 0 }; 
    
    data.last.num = 0;
    data.last.dt = dt;
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
    
    time_t utm = now();
    dt_t dt = {
        .y      = year(utm),
        .m      = month(utm),
        .d      = day(utm),
        .hh     = hour(utm),
        .mm     = minute(utm),
        .ss     = second(utm),
        .dow    = weekday(utm),
    };
    data.last.dt = dt;
    
    auto li = jmpLogItem();
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
    
    auto li = jmpLogItem();
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
    
    data.last.end = jmpLogItem();
    
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
log_item_t jmpLogItem() {
    auto &ac = altCalc();
    auto &gps = gpsGet();
    
    log_item_t li = {
        .mill   = millis(),
        .alt    = ac.alt(),
        .vspeed = ac.speed(),
        .state  = ac.state(),
        .direct = ac.direct(),
        .lat    = gps.location.lat(),
        .lng    = gps.location.lng(),
        .hspeed = gps.speed.mps(),
        .hang   = gps.course.deg(),
        .sat    = gps.satellites.value(),
    };
    
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
