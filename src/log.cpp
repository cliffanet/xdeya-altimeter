
#include "log.h"

#ifdef FWVER_DEBUG

#include "gps/proc.h"
#include "clock.h"

#include <Arduino.h>

/* ------------------------------------------------------------------------------------------- *
 *  Логирование в консоль
 * ------------------------------------------------------------------------------------------- */
static char pathToFileName_S[15];
const char * pathToFileName_P(const char * path) {
    int l = strlen_P(path);
    char s[l+1];
    strcpy_P(s, path);
    strncpy(pathToFileName_S, pathToFileName(s), sizeof(pathToFileName_S));
    pathToFileName_S[sizeof(pathToFileName_S) - 1] = '\0';
    return pathToFileName_S;
}

static void vtxtlog(const char *s, va_list ap) {
    if (gpsDirect())
        return;
    
    int len = vsnprintf(NULL, 0, s, ap), sbeg = 0;
    char str[len+60]; // +48=dt +12=debug mill/tick
    
    uint64_t ms = utm() / 1000;
    uint32_t d = ms / (3600*24*1000);
    uint8_t h = (ms - d*3600*24*1000) / (3600*1000);
    uint8_t m = (ms - d*3600*24*1000 - h*3600*1000) / (60*1000);
    uint8_t ss = (ms - d*3600*24*1000 - h*3600*1000 - m*60*1000) / 1000;
    ms -= d*3600*24*1000 + h*3600*1000 + m*60*1000 + ss*1000;
    sbeg = snprintf_P(str, 32, PSTR("%3ud %2u:%02u:%02u.%03llu"), d, h, m, ss, ms);
    
    vsnprintf(str+sbeg, len+1, s, ap);
    
    Serial.println(str);
}

void conslog_P(const char *s, ...) {
    va_list ap;
    char str[strlen_P(s)+1];

    strcpy_P(str, s);
    
    va_start (ap, s);
    vtxtlog(str, ap);
    va_end (ap);
}

#endif // FWVER_DEBUG
