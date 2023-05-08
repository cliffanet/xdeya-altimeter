
#include "jump.h"
#include "../jump/logbook.h"
#include "../clock.h"
#include "../monlog.h"

#include "esp_system.h"

/* ------------------------------------------------------------------------------------------- *
 *  Пишем в простой лог прыжки
 * ------------------------------------------------------------------------------------------- */

ConfigJump jmp;

ConfigJump::ConfigJump() :
    Config(PSTR(CFG_JUMP_NAME), CFG_JUMP_ID, CFG_JUMP_VER)
{
    log_item_t li = { 0 };
    li.state = ACST_INIT;
    li.direct = ACDIR_INIT;
    tm_t tm = { 0 }; 
    
    data.last.num = 0;
    data.last.key = 0;
    data.last.tm = tm;
    data.last.toff = li;
    data.last.beg = li;
    data.last.cnp = li;
    data.last.end = li;
}

/* ------------------------------------------------------------------------------------------- *
 *  Генерирует при необходимости случайный ключ прыга, и возвращает его
 * ------------------------------------------------------------------------------------------- */
uint32_t ConfigJump::key() {
    // Эта функция вызывается только снаружи
    // И пока не начался прыг (свободное падение), мы каждый раз перегенерируем его
    if ((data.last.key == 0) || (data.state < LOGJMP_BEG))
        keygen();
    
    return data.last.key;
}
bool ConfigJump::keygen() {
    // сброс ключа снаружи - только если он не используется тут
    if ((data.last.key != 0) && (data.state != LOGJMP_NONE))
        return false;
    
    data.last.key = esp_random();
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  старт
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::toff(uint16_t old) {
    if (data.state != LOGJMP_NONE)
        return false;
    
    data.state = LOGJMP_TOFF;
    
    data.last.num = data.count;
    
    data.last.tm = tmNow(jmpPreInterval(old));
#ifdef FWVER_DEBUG
    MONITOR("toff[%d]: %d.%02d.%d %d:%02d:%02d", old, 
        data.last.tm.day, data.last.tm.mon, data.last.tm.year,
        data.last.tm.h, data.last.tm.m, data.last.tm.s);
    auto cur = jmpPreCursor()-old;
    MONITOR("cur toff: %d(%d) / %d / %d (%d)", cur.value(), jmpPreCursor().value(), cur.capacity(), cur.size());
#endif // FWVER_DEBUG
    
    data.last.toff = jmpPreLog(old);
    data.last.toff.tmoffset = 0;
    data.last.toff.msave = utm() / 1000;
    
    return save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Начало свободного падения - инициализируем прыжок
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::beg(uint16_t old) {
    if ((data.state < LOGJMP_TOFF) && !toff(old))
        return false;
    
    data.count++;
    data.last.num = data.count;
    
    auto tm = tmNow(jmpPreInterval(old));
    MONITOR("beg[%d]: %d.%02d.%d %d:%02d:%02d", old, tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s);
    if (data.state == LOGJMP_TOFF)
        data.last.toff.tmoffset = tmInterval(data.last.tm, tm);
    data.last.tm = tm;
#ifdef FWVER_DEBUG
    auto cur = jmpPreCursor()-old;
    MONITOR("cur beg: %d(%d) / %d / %d (%d)", cur.value(), jmpPreCursor().value(), cur.capacity(), cur.size());
#endif // FWVER_DEBUG
    
    if (data.last.key == 0)
        // перегенерируем key только если он нулевой (не используется никем)
        keygen();
    data.state = LOGJMP_BEG;
    
    data.last.beg = jmpPreLog(old);
    data.last.beg.tmoffset = 0;
    data.last.beg.msave = utm() / 1000;
    data.last.cnp = data.last.beg;
    data.last.end = data.last.beg;
    
    return true;//save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Под куполом, сохраняем промежуточные значения
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::cnp(uint16_t old) {
    if ((data.state < LOGJMP_BEG) && !beg(old))
        return false;
    
    data.state = LOGJMP_CNP;
    
    auto tm = tmNow(jmpPreInterval(old));
    MONITOR("cnp[%d]: %d.%02d.%d %d:%02d:%02d", old, tm.day, tm.mon, tm.year, tm.h, tm.m, tm.s);
    
    data.last.cnp = jmpPreLog(old);
    data.last.cnp.tmoffset = tmInterval(data.last.tm, tm);
    data.last.cnp.msave = utm() / 1000;
    data.last.end = data.last.cnp;
    
    return true;//save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Окончание прыга, пишем в логбук
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::end() {
    if (data.state < LOGJMP_BEG) {
        data.state = LOGJMP_NONE;
        return false;
    }
    
    data.state = LOGJMP_NONE;
    MONITOR("jmp end");
    
    data.last.end = jmpPreLog();
    data.last.end.tmoffset = tmIntervalToNow(data.last.tm);
    data.last.end.msave = utm() / 1000;
    
    if (!save(true))
        return false;
    
    FileLogBook flb;
    if (!flb.append(data.last))
        return false;
    flb.close();

    data.last.key = 0;
    
    return true;
}
