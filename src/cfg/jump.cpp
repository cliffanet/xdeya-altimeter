
#include "jump.h"
#include "../file/log.h"
#include "../clock.h"

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
        data.last.key = esp_random();
    
    return data.last.key;
}
void ConfigJump::keyreset() {
    // сброс ключа снаружи - только если он не используется тут
    if (data.state == LOGJMP_NONE)
        data.last.key = 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  старт
 * ------------------------------------------------------------------------------------------- */
bool ConfigJump::toff(uint16_t old) {
    if (data.state != LOGJMP_NONE)
        return false;
    
    data.state = LOGJMP_TOFF;
    
    data.last.num = data.count;
    data.last.tm = tmNow(jmpPreLogInterval(old));
    
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
    
    auto tm = tmNow(jmpPreLogInterval(old));
    if (data.state == LOGJMP_TOFF)
        data.last.toff.tmoffset = tmInterval(data.last.tm, tm);
    data.last.tm = tm;
    
    data.state = LOGJMP_BEG;
    key(); // перегенерируем key только если он нулевой (не используется никем)
    
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
    if ((data.state < LOGJMP_BEG) && !beg(old))
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
    if (data.state < LOGJMP_BEG) {
        data.state = LOGJMP_NONE;
        return false;
    }
    
    data.state = LOGJMP_NONE;
    
    data.last.end = jmpPreLog();
    data.last.end.tmoffset = tmIntervalToNow(data.last.tm);
    data.last.end.msave = utm() / 1000;
    
    if (!save(true))
        return false;
    
    if (!logAppend(PSTR(JMPLOG_SIMPLE_NAME), data.last, JMPLOG_SIMPLE_ITEM_COUNT, JMPLOG_SIMPLE_FILE_COUNT))
        return false;

    data.last.key = 0;
    
    return true;
}
