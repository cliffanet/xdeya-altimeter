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


/* ------------------------------------------------------------------------------------------- *
 *  Дописывание в конец
 * ------------------------------------------------------------------------------------------- */
template <typename T>
bool logAppend(const char *_fname, const T &data, size_t maxrcnt, uint8_t count) {
    char fname[36];
    
    strncpy_P(fname, _fname, 30);
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

#define logRCount(fname, type)      (logSize(fname, type)/sizeof(type))
#define logRCountFull(fname, type)  (logSizeFull(fname, type)/sizeof(type))

#endif // _logfile_H
