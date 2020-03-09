/*
    LOG files functions
*/

#ifndef _logfile_H
#define _logfile_H

#include <Arduino.h>

#include "FS.h"
#include <SPIFFS.h>

#define DISKFS SPIFFS

#define LOGFILE_SUFFIX  ".%02d"

bool logExists(const char *_fname, uint8_t num = 1);
size_t logSize(const char *_fname, uint8_t num = 1);
size_t logSizeFull(const char *_fname);
bool logRotate(const char *_fname, uint8_t count);


#define LOG_MGC1        0xe4
#define LOG_MGC2        0x7a

template <typename T>
struct __attribute__((__packed__)) log_item_s {
    uint8_t mgc1 = LOG_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    T data;
    uint8_t mgc2 = LOG_MGC2;
    log_item_s() { };
    log_item_s(const T &_data) : data(_data) { };
};

/* ------------------------------------------------------------------------------------------- *
 *  Дописывание в конец
 * ------------------------------------------------------------------------------------------- */
template <typename T>
bool logAppend(const char *_fname, const T &data, size_t maxrcnt, uint8_t count) {
    char fname[36];
    
    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    const byte flen = strlen(fname);
    sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), 1);
    
    if (DISKFS.exists(fname)) {
        File fh = DISKFS.open(fname);
        if (!fh) return false;
        
        auto sz = fh.size();
        fh.close();
        
        if ((sz + sizeof(T)) > (maxrcnt*sizeof(T)))
            if (!logRotate(_fname, count))
                return false;
    }
    
    File fh = DISKFS.open(fname, FILE_APPEND);
    if (!fh)
        return false;
    
    auto sz = fh.write(reinterpret_cast<const uint8_t *>(&data), sizeof(T));
    fh.close();
    
    return sz == sizeof(T);
}

#define logRCount(fname, type)      (logSize(fname)/sizeof(type))
#define logRCountFull(fname, type)  (logSizeFull(fname)/sizeof(type))

#endif // _logfile_H
