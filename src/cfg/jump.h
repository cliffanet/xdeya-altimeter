/*
    Altimeter
*/

#ifndef _cfg_jump_H
#define _cfg_jump_H

#include "../../def.h"
#include "main.h"
#include "../altcalc.h"
#include "../clock.h"

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
typedef struct __attribute__((__aligned__(64), __packed__)) {
    int32_t     tmoffset;   // время от начала измерений        (ms)
    uint16_t    flags;      // флаги: валидность
    uint8_t     state;      // статус высотомера (земля, подъём, падение, под куполом)
    uint8_t     direct;     // направление движения по высоте
    uint16_t    alt;        // высота по барометру              (m)
    uint16_t    altspeed;   // скорость падения по барометру    (cm/s)
    uint32_t    lon;        // Longitude                        (deg * 10^7)
    uint32_t    lat;        // Latitude                         (deg * 10^7)
    uint32_t    hspeed;     // Ground speed                     (cm/s)
    int16_t     heading;    // направление движения             (deg)
    int16_t     gpsalt;     // высота по GPS (над ур моря)      (m)
    uint32_t    vspeed;     // 3D speed                         (cm/s)
    uint8_t     sat;        // количество найденных спутников
    uint8_t     _;
    uint16_t    batval;     // raw-показания напряжения на батарее
} log_item_t;

#define LI_FLAG_GPS_VALID   0x0001
#define LI_FLAG_GPS_VLOC    0x0002
#define LI_FLAG_GPS_VVERT   0x0004
#define LI_FLAG_GPS_VSPEED  0x0008
#define LI_FLAG_GPS_VHEAD   0x0010
#define LI_FLAG_GPS_VTIME   0x0020
#define LI_FLAG_BTN_UP      0x2000
#define LI_FLAG_BTN_SEL     0x4000
#define LI_FLAG_BTN_DOWN    0x8000


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
    private:
        tm_val_t tmval;
};

log_item_t jmpLogItem(const tm_val_t &tmval);
void jmpProcess();
void jmpReset();

extern ConfigJump jmp;

#endif // _cfg_jump_H
