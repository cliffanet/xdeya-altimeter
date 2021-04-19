/*
    Log
*/

#ifndef _log_H
#define _log_H

#include "../def.h"

const char * pathToFileName_P(const char * path);
#define LOG_FMT(s)        PSTR(" [%s:%u] %s(): " s), pathToFileName_P(PSTR(__FILE__)), __LINE__, __FUNCTION__
//#define LOG_FMT(s)        PSTR(" [%s:%u] %s(): " s), __FILE__, __LINE__, __FUNCTION__

void conslog_P(const char *s, ...);
#define CONSOLE(txt, ...) conslog_P(LOG_FMT(txt), ##__VA_ARGS__)

#endif // _log_H
