/*
    Monitor Log
*/

#ifndef _mainlog_H
#define _mainlog_H

#include "../def.h"

#ifdef FWVER_DEBUG

#include "log.h"

#define LOG_FMT(s)        PSTR("[%s:%u] " s), pathToFileName_P(PSTR(__FILE__)), __LINE__

void monlog_P(const char *s, ...);
#define MONITOR(txt, ...) monlog_P(LOG_FMT(txt), ##__VA_ARGS__)

void setViewMainAltChart();
void altchartadd(int val, int avg);
void altchartmode(int m, uint16_t cnt);

#else // FWVER_DEBUG

#define MONITOR(txt, ...)

#define altchartadd(val, avg)
#define altchartmode(m, cnt)

#endif // FWVER_DEBUG

#endif // _mainlog_H
