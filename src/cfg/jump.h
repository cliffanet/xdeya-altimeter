/*
    Altimeter
*/

#ifndef _cfg_jump_H
#define _cfg_jump_H

#include "main.h"
#include "../jump/logbook.h"

typedef enum {
    LOGJMP_NONE,
    LOGJMP_TOFF,
    LOGJMP_BEG,
    LOGJMP_CNP
} log_jmp_state_t;


#define CFG_JUMP_ID     4
#define CFG_JUMP_VER    1
#define CFG_JUMP_NAME   "jmp"

//  Сохранение информации о прыжках
typedef struct __attribute__((__packed__)) {
    uint32_t count = 0;                      // Сколько всего прыжков у владельца
    FileLogBook::item_t last;
    log_jmp_state_t state = LOGJMP_NONE;
} cfg_jump_t;


class ConfigJump : public Config<cfg_jump_t> {
    public:
        ConfigJump();
        uint32_t key();
        bool keygen();
        bool toff(uint16_t old = 0);
        bool beg(uint16_t old = 0);
        bool cnp(uint16_t old = 0);
        bool end();
        
        uint32_t        count() const { return data.count; }
        log_jmp_state_t state() const { return data.state; }
        const FileLogBook::item_t & last() const{ return data.last; }
};

extern ConfigJump jmp;

#endif // _cfg_jump_H
