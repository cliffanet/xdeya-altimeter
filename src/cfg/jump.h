/*
    Altimeter
*/

#ifndef _cfg_jump_H
#define _cfg_jump_H

#include "../../def.h"
#include "main.h"
#include "../jump/proc.h"

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
    private:
        tm_val_t tmval;
};

extern ConfigJump jmp;

#endif // _cfg_jump_H
