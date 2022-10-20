/*
    Clock
*/

#ifndef _clock_H
#define _clock_H

#include "../def.h"

#include <stdint.h>


/* ------------------------------------------------------------------------------------------- *
 *  операции со временем в формате tm_t
 * ------------------------------------------------------------------------------------------- */
uint64_t utm();
uint64_t utick();
uint64_t utm_diff(uint64_t utick_prev, uint64_t &utick_curr);
uint64_t utm_diff(uint64_t utick_prev);
uint32_t utm_diff32(uint32_t utick_prev, uint32_t &utick_curr);
uint32_t utm_diff32(uint32_t utick_prev);



/* ------------------------------------------------------------------------------------------- *
 *  таймеры
 * ------------------------------------------------------------------------------------------- */
typedef enum {
    TMCNT_UPTIME   = 0,
    TMCNT_NOFLY,
    TMCNT_FAIL
} tm_counter_t;
void tmcntReset(tm_counter_t id, bool enable);
bool tmcntEnabled(tm_counter_t id);
uint32_t tmcntInterval(tm_counter_t id);
uint32_t tmcntIntervEn(tm_counter_t id);
void tmcntUpdate();

/* ------------------------------------------------------------------------------------------- *
 *  операции со временем в формате tm_t
 * ------------------------------------------------------------------------------------------- */
typedef struct __attribute__((__packed__)) tm_s {
	uint16_t year;     // Year                         (1999..2099)
	uint8_t  mon;      // Month                        (1..12)
	uint8_t  day;      // Day of month                 (1..31)
	uint8_t  h;        // Hour of day                  (0..23)
	uint8_t  m;        // Minute of hour               (0..59)
	uint8_t  s;        // Second of minute             (0..59)
    uint8_t  tick;     // tick count (tick = TIME_TICK_INTERVAL ms)
    
    bool operator== (const struct tm_s & tm) {
        return
            (this == &tm) ||
            (
                (this->year == tm.year) && (this->mon == tm.mon) && (this->day == tm.day) &&
                (this->h == tm.h) && (this->m == tm.m) && (this->s == tm.s)
            );
    };
    bool operator!= (const struct tm_s & tm) { return !(*this == tm); };
} tm_t;

#define TIME_TICK_INTERVAL      100

int32_t tmInterval(const tm_t &tmbeg, const tm_t &tmend);
int32_t tmIntervalToNow(const tm_t &tm);

/* ------------------------------------------------------------------------------------------- *
 *  внешние часы
 * ------------------------------------------------------------------------------------------- */

bool tmValid();
tm_t &tmNow();
tm_t tmNow(uint32_t earlerms);

// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL    1200

void clockInit();
void clockForceAdjust(uint16_t interval = 0);

void clockProcess();

#endif // _clock_H
