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

/* ------------------------------------------------------------------------------------------- *
 *  Количество файлов
 * ------------------------------------------------------------------------------------------- */
int logCount(const char *_fname);

/* ------------------------------------------------------------------------------------------- *
 *  Размер файла
 * ------------------------------------------------------------------------------------------- */
size_t logSize(const char *_fname, uint8_t num = 1);
size_t logSizeFull(const char *_fname);

#define logRCount(fname, type)      (logSize(fname)/sizeof(type))
#define logRCountFull(fname, type)  (logSizeFull(fname)/sizeof(type))

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
bool logAppend(const char *_fname, const uint8_t *data, uint16_t dsz, size_t maxrcnt, uint8_t count);

template <typename T>
bool logAppend(const char *_fname, const T &data, size_t maxrcnt, uint8_t count) {
    return logAppend(_fname, reinterpret_cast<const uint8_t *>(&data), sizeof(T), maxrcnt, count);
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение записи с индексом i с конца (при i=0 - самая последняя запись, при i=1 - предпоследняя)
 * ------------------------------------------------------------------------------------------- */
bool logRead(uint8_t *data, uint16_t dsz, const char *_fname, size_t index = 0);

template <typename T>
bool logRead(T &data, const char *_fname, size_t index = 0) {
    return
        logRead(reinterpret_cast<uint8_t *>(&data), sizeof(T), _fname, index) &&
        (data.mgc1 == LOG_MGC1) && (data.mgc2 == LOG_MGC2);
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение всех записей, начиная с индекса ibeg (при ibeg=0 - самая первая запись),
 *  Возвращает индекс следующей позиции после крайней прочитанной
 * ------------------------------------------------------------------------------------------- */
int32_t logFileRead(bool (*hnd)(const uint8_t *data), uint16_t dsz, const char *_fname, uint16_t fnum = 1, size_t ibeg = 0);


template <typename T>
int32_t logFileRead(bool (*hnd)(const T *data), const char *_fname, uint16_t fnum = 1, size_t ibeg = 0) {
    return
        logFileRead(reinterpret_cast<bool (*)(const uint8_t *data)>(hnd), sizeof(T), _fname, fnum, ibeg);
}


/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма файла
 * ------------------------------------------------------------------------------------------- */
typedef struct  __attribute__((__packed__)) logchs_s {
    uint32_t    cs;
    uint32_t    sz;
    
    bool operator== (const struct logchs_s & cks) {
        return (this == &cks) || ((this->cs == cks.cs) && (this->sz == cks.sz));
    };
    operator bool() { return (cs != 0) && (sz != 0); }
} logchs_t;
logchs_t logChkSum(size_t dsz, const char *_fname, uint8_t num = 1);
uint8_t logFind(const char *_fname, size_t dsz, const logchs_t &cks);


/* ------------------------------------------------------------------------------------------- *
 *  Удаление файла
 * ------------------------------------------------------------------------------------------- */
int logRemoveLast(const char *_fname, bool removeFirst = false);
int logRemoveAll(const char *_fname, bool removeFirst = false);

#endif // _logfile_H
