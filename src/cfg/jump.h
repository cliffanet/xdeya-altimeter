/*
    Altimeter
*/

#ifndef _cfg_jump_H
#define _cfg_jump_H

#include "main.h"
#include "../altcalc.h"

// имя файла для хранения простых логов
#define JMPLOG_SIMPLE_NAME          "logsimple"
// Сколько прыжков в одном файле
#define JMPLOG_SIMPLE_ITEM_COUNT    50
// Сколько файлов прыгов максимум
#define JMPLOG_SIMPLE_FILE_COUNT    5

// Количество тиков высотомера с сохранением направления ACDIR_DOWN
// после которого надо запускать автоматически запись прыга
#define JMP_ALTI_DIRDOWN_COUNT  50

// Один из элементов в длинном логбуке (несколько раз в сек)
typedef struct __attribute__((__packed__)) {
    uint32_t    mill;
    float       alt;
    float       vspeed;
    ac_state_t  state;
    ac_direct_t direct;
    double      lat;
    double      lng;
    double      hspeed;
    uint16_t    hang;
    uint8_t     sat;
    uint8_t     _;
} log_item_t;


typedef struct __attribute__((__packed__)) {
    uint16_t  y;
    uint8_t   m;
    uint8_t   d;
    uint8_t   hh;
    uint8_t   mm;
    uint8_t   ss;
    uint8_t   dow;
} dt_t;

// Один прыг в простом логбуке, где запоминаются только данные на начало, середину и самое окончание прыга
typedef struct __attribute__((__packed__)) {
    uint32_t    num;
    dt_t        dt;
    log_item_t  beg;
    log_item_t  cnp;
    log_item_t  end;
} log_jmp_t;

typedef enum {
    LOGJMP_NONE,
    LOGJMP_BEG,
    LOGJMP_CNP
} log_jmp_state_t;

//  Сохранение информации о прыжках
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = CFG_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = CFG_JUMP_VER;
    
    uint32_t count = 0;                      // Сколько всего прыжков у владельца
    log_jmp_t last;
    log_jmp_state_t state = LOGJMP_NONE;
    uint8_t mgc2 = CFG_MGC2;
} cfg_jump_t;


class ConfigJump : public Config<cfg_jump_t> {
    public:
        ConfigJump();
        bool beg();
        bool cnp();
        bool end();
        
        uint8_t         count() const { return data.count; }
        log_jmp_state_t state() const { return data.state; }
        const log_jmp_t & last() const{ return data.last; }
};

log_item_t jmpLogItem();
void jmpProcess();
void jmpReset();

extern ConfigJump jmp;

#endif // _cfg_jump_H
