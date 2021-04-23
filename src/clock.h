/*
    Clock
*/

#ifndef _clock_H
#define _clock_H

#include "../def.h"

#if HWVER >= 3
#define CLOCK_PIN_INT       14
#endif

// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   (TIME_ADJUST_INTERVAL*3)
//bool timeOk();
    
typedef struct {
	uint16_t year;     // Year                         (1999..2099)
	uint8_t  mon;      // Month                        (1..12)
	uint8_t  day;      // Day of month                 (1..31)
	uint8_t  h;        // Hour of day                  (0..23)
	uint8_t  m;        // Minute of hour               (0..59)
	uint8_t  s;        // Second of minute             (0..59)
    bool     valid;
} tm_t;

volatile tm_t &tmNow();

void clockInit();
void clockForceAdjust();
void clockProcess();

#endif // _clock_H