
#include "logbook.h"
#include "../eeprom.h"
#include "../altimeter.h"
#include "../gps.h"

#include <vector>
#include <EEPROM.h>

static log_running_t logrun = LOGRUN_NONE;
static std::vector<log_item_t> logfull;
enum {
    JMP_NONE,
    JMP_FREEFALL,
    JMP_CANOPY
} jmp = JMP_NONE;
log_item_t jmpbeg, jmpcnp;



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
    }
    
    if ((jmp > JMP_FREEFALL) && (ac.state() == ACST_CANOPY)) {
        // Переход в режим CNP после начала прыга,
        // Дальше только окончание прыга может быть, даже если начнётся снова FF,
        // Для jmp только такой порядок переходов,
        // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
        jmpcnp = li;
        jmp = JMP_CANOPY;
        // Сохраняем промежуточный результат
    }
    
    if ((jmp > JMP_NONE) && (ac.state() == ACST_GROUND)) {
        // Прыг закончился совсем, сохраняем результат
        if (jmp == JMP_FREEFALL) // Был пропущен момент перехода на JMP_CANOPY
            jmpcnp = li;
        jmp = JMP_NONE;
        // Сохраняем
    }
    
    Serial.println(logfull.size());
}
