
#include "jump.h"
#include "../file/log.h"

/* ------------------------------------------------------------------------------------------- *
 *  Пишем в простой лог прыжки
 * ------------------------------------------------------------------------------------------- */

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
