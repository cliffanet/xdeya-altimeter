
#include "jump.h"
#include "../file/log.h"
#include "../clock.h"

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
    data.last.tm = tm;
    data.last.beg = li;
    data.last.cnp = li;
    data.last.end = li;
}

/* ------------------------------------------------------------------------------------------- *
 *  Старт прыга, инициируем новый прыжок
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::beg(uint16_t old) {
    if (data.state != LOGJMP_NONE)
        return false;
    
    data.count++;
    data.state = LOGJMP_BEG;
    
    data.last.num = data.count;
    data.last.tm = tmNow(jmpPreLogInterval(old));
    
    data.last.beg = jmpPreLog(old);
    data.last.beg.tmoffset = 0;
    data.last.beg.msave = utm() / 1000;
    data.last.cnp = data.last.beg;
    data.last.end = data.last.beg;
    
    return save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Под куполом, сохраняем промежуточные значения
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::cnp(uint16_t old) {
    if (data.state == LOGJMP_NONE) {
        beg(old);
        data.state = LOGJMP_CNP;
        return save(true);
    }
    
    if (data.state != LOGJMP_BEG)
        return false;
    
    data.state = LOGJMP_CNP;
    
    auto tm = tmNow(jmpPreLogInterval(old));
    
    data.last.cnp = jmpPreLog(old);
    data.last.cnp.tmoffset = tmInterval(data.last.tm, tm);
    data.last.cnp.msave = utm() / 1000;
    data.last.end = data.last.cnp;
    
    return save(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Окончание прыга, пишем в логбук
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::end() {
    if (data.state == LOGJMP_NONE)
        return false;
    
    data.state = LOGJMP_NONE;
    
    data.last.end = jmpPreLog();
    data.last.end.tmoffset = tmIntervalToNow(data.last.tm);
    data.last.end.msave = utm() / 1000;
    
    if (!save(true))
        return false;
    
    if (!logAppend(PSTR(JMPLOG_SIMPLE_NAME), data.last, JMPLOG_SIMPLE_ITEM_COUNT, JMPLOG_SIMPLE_FILE_COUNT))
        return false;
    
    return true;
}
