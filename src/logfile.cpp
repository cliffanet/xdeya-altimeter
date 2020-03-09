
#include "logfile.h"

/* ------------------------------------------------------------------------------------------- *
 *  Существование файла
 * ------------------------------------------------------------------------------------------- */
bool logExists(const char *_fname, uint8_t num) {
    char fname[37];
    
    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    const byte flen = strlen(fname);
    sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), num);
    
    return DISKFS.exists(fname);
}

/* ------------------------------------------------------------------------------------------- *
 *  Размер файла
 * ------------------------------------------------------------------------------------------- */
size_t logSize(const char *_fname, uint8_t num) {
    char fname[37];
    
    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    const byte flen = strlen(fname);
    sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), num);
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return -1;
    
    auto sz = fh.size();
    fh.close();
        
    return sz;
}

size_t logSizeFull(const char *_fname) {
    uint8_t n = 1;
    char fname[37];
    size_t sz = 0;
    
    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    const byte flen = strlen(fname);
    
    sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), n);
    
    while (DISKFS.exists(fname)) {
        File fh = DISKFS.open(fname);
        if (!fh)
            continue;
        
        auto sz1 = fh.size();
        fh.close();
        
        if (sz1 > 0)
            sz += sz1;
        
        n++;
        sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), n);
    }
    
    return sz;
}

/* ------------------------------------------------------------------------------------------- *
 *  Ротирование файлов
 * ------------------------------------------------------------------------------------------- */
bool logRotate(const char *_fname, uint8_t count) {
    uint8_t n = 1;
    char fname[37], fname1[37];

    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    strcpy(fname1, fname);
    const byte flen = strlen(fname);
    
    if (count == 0)
        count = 255;

    // ищем первый свободный слот
    while (1) {
        sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), n);
        if (!DISKFS.exists(fname)) break;
        if (n < count) {
            n++; // пока ещё не достигли максимального номера файла, продолжаем
            continue;
        }
        // достигнут максимальный номер файла
        // его надо удалить и посчитать первым свободным слотом
        if (!DISKFS.remove(fname)) {
            Serial.print(F("Can't remove file '"));
            Serial.print(fname);
            Serial.println(F("'"));
            return false;
        }
        Serial.print(F("file '"));
        Serial.print(fname);
        Serial.println(F("' removed"));
        break;
    }

    // теперь переименовываем все по порядку
    while (n > 1) {
        sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), n-1);
        sprintf_P(fname1+flen, PSTR(LOGFILE_SUFFIX), n);
        
        if (!DISKFS.rename(fname, fname1)) {
            Serial.print(F("Can't rename file '"));
            Serial.print(fname);
            Serial.print(F("' to '"));
            Serial.print(fname1);
            Serial.println(F("'"));
            return false;
        }
        Serial.print(F("file '"));
        Serial.print(fname);
        Serial.print(F("' renamed to '"));
        Serial.print(fname1);
        Serial.println(F("'"));
        
        n--;
    }

    return true;
}


