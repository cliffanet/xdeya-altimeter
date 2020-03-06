
#include "logbook.h"
#include "../eeprom.h"
#include "../altimeter.h"
#include "../gps.h"

#include <vector>
#include <EEPROM.h>
#include <TimeLib.h>

static log_running_t logrun = LOGRUN_NONE;
static std::vector<log_item_t> logfull;
enum {
    JMP_NONE,
    JMP_FREEFALL,
    JMP_CANOPY
} jmp = JMP_NONE;
log_item_t jmpbeg, jmpcnp;

/* ------------------------------------------------------------------------------------------- *
 *  Добавление ещё одного прыга в простой логбук (eeprom)
 *  В качестве beg,cnp,end пишется одно и то же - стартовая позиция,
 *  А в соответствующие моменты будут обновлены и они
 * ------------------------------------------------------------------------------------------- */
static void jmpInc(const log_item_t &beg) {
    cfg.jmpcount++;
    cfgSave();
    
    EEPROMClass eep(EEPROM_LOG_SIMPLE_NAME, 0);
    eep.begin(sizeof(log_simple_t));
    auto *ls = (log_simple_t *)eep.getDataPtr();

    if (!EEPROM_LOG_SIMPLE_VALID(*ls)) {
        log_simple_t log;
        *ls = log;
    }
    if (ls->cnt > LOG_SIMPLE_COUNT)
        ls->cnt = 0;
    
    if (ls->cnt >= LOG_SIMPLE_COUNT) {
        for (int i=1; i < LOG_SIMPLE_COUNT; i++)
            ls->jmp[i-1] = ls->jmp[i];
    }
    else {
        ls->cnt++;
    }
    
    auto &log = ls->jmp[ls->cnt-1];
    log.num = cfg.jmpcount;
    
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
    log.dt = dt;
    
    log.beg = beg;
    log.cnp = beg;
    log.end = beg;
    
    eep.commit();
    eep.end();
}

/* ------------------------------------------------------------------------------------------- *
 *  Обновление данных по текущему прыгу в простом логбуке (eeprom)
 * ------------------------------------------------------------------------------------------- */
static void jmpUpd(const log_item_t &cnp, const log_item_t &end) {
    EEPROMClass eep(EEPROM_LOG_SIMPLE_NAME, 0);
    eep.begin(sizeof(log_simple_t));
    auto *ls = (log_simple_t *)eep.getDataPtr();

    if (!EEPROM_LOG_SIMPLE_VALID(*ls) || (ls->cnt <= 0) || (ls->cnt > LOG_SIMPLE_COUNT))
        return;
    
    auto &log = ls->jmp[ls->cnt-1];
    if (log.num != cfg.jmpcount)
        return;
    
    log.cnp = cnp;
    log.end = end;
    
    eep.commit();
    eep.end();
}

/* ------------------------------------------------------------------------------------------- *
 *  Общий процессинг (на каждый тик тут приходится два тика высотомера и один тик жпс)
 * ------------------------------------------------------------------------------------------- */
void logProcess() {
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
    
    if ((jmp == JMP_NONE) &&
        ((ac.state() == ACST_FREEFALL) || (ac.state() == ACST_CANOPY)) &&
        (ac.direct() == ACDIR_DOWN) &&
        (ac.dircnt() > LOG_ALTI_DIRDOWN_COUNT)) {
        // Включаем запись лога прыга
        jmp = JMP_FREEFALL; // Самое начало прыга помечаем в любом случае как FF,
                              // т.к. из него можно перейти в CNP, но не обратно (именно для jmp)
        if (logrun == LOGRUN_NONE)
            logrun = LOGRUN_ALTI;
        jmpbeg = logfull.size() > 0 ? logfull.front() : li;
        jmpInc(jmpbeg);
    }
    
    if ((logrun == LOGRUN_NONE) && (logfull.size() >= LOG_COUNT_MIN)) {
        // Чтобы не заниматься перевыделением памяти (так будет например в случае queue, при удалении вначале и добавлении в конец),
        // Используем сдвиг элементов в начало, пока нам надо поддерживать размер не больше, чем LOG_COUNT_MIN
        for (int i = 1; i < logfull.size(); i++)
            logfull[i-1] = logfull[i];
        logfull.back() = li;
    }
    else if (logfull.size() < LOG_COUNT_MAX) {
        logfull.push_back(li);
    }
    
    if ((logfull.size() >= LOG_COUNT_MAX) ||                        // Закончилось место в логе, надо остановить
        ((logrun == LOGRUN_ALTI) && (ac.state() == ACST_GROUND))) { // Либо мы приземлились
        logrun = LOGRUN_NONE;
        // сохраняем лог
        logfull.clear();
    }
    
    if ((jmp > JMP_FREEFALL) && (ac.state() == ACST_CANOPY)) {
        // Переход в режим CNP после начала прыга,
        // Дальше только окончание прыга может быть, даже если начнётся снова FF,
        // Для jmp только такой порядок переходов,
        // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
        jmpcnp = li;
        jmp = JMP_CANOPY;
        // Сохраняем промежуточный результат
        jmpUpd(li, li);
    }
    
    if ((jmp > JMP_NONE) && (ac.state() == ACST_GROUND)) {
        // Прыг закончился совсем, сохраняем результат
        if (jmp == JMP_FREEFALL) // Был пропущен момент перехода на JMP_CANOPY
            jmpcnp = li;
        jmp = JMP_NONE;
        // Сохраняем
        jmpUpd(jmpcnp, li);
    }
    
    //Serial.println(logfull.size());
}
