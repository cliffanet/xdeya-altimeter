/*
    Altimeter
*/

#ifndef _logbook_H
#define _logbook_H

#include <Arduino.h>
#include "../altcalc.h"

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

// Один прыг в простом логбуке, где запоминаются только данные на начало, середину и самое окончание прыга
typedef struct __attribute__((__packed__)) {
    log_item_t beg;
    log_item_t cnp;
    log_item_t end;
} log_jmp_t;

// Минимальное количество записей логбука (поддерживается, пока не началась запись прыжка)
// Нужно, чтобы отловить самое начало прыжка, т.к. для уверенного понимания, что прыг начался,
// делается задержка на сохранение движения вниз
#define LOG_COUNT_MIN   30
#define LOG_COUNT_MAX   10000

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
