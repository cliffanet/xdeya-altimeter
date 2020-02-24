/*
    Altimeter
*/

#ifndef _logbook_H
#define _logbook_H

#include <Arduino.h>
#include "../altcalc.h"
#include "../eeprom.h"

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


#define LOG_SIMPLE_COUNT 50
#define EEPROM_LOG_SIMPLE_NAME          "logsmp"
#define EEPROM_LOG_SIMPLE_VALID(var)    (((var).mgc1 == EEPROM_MGC1) && ((var).ver == 1) && ((var).mgc2 == EEPROM_MGC2))

typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = EEPROM_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = 1;
    uint16_t cnt = 0;                           // Используемых ячеек
    log_jmp_t jmp[LOG_SIMPLE_COUNT];            // Ячейки хранения
    uint8_t mgc2 = EEPROM_MGC2;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
} log_simple_t;

// Минимальное количество записей логбука (поддерживается, пока не началась запись прыжка)
// Нужно, чтобы отловить самое начало прыжка, т.к. для уверенного понимания, что прыг начался,
// делается задержка на сохранение движения вниз
#define LOG_COUNT_MIN   30
#define LOG_COUNT_MAX   7000

// На каждые два тика высотомера приходится только один тик логбука

// Количество тиков высотомера с сохранением направления ACDIR_DOWN
// после которого надо запускать автоматически запись прыга
#define LOG_ALTI_DIRDOWN_COUNT  50

typedef enum {
    LOGRUN_NONE,
    LOGRUN_FORCE,
    LOGRUN_ALTI
} log_running_t;

void logProcess();

#endif // _logbook_H
