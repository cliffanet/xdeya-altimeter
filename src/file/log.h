/*
    LOG files functions
*/

#ifndef _file_log_H
#define _file_log_H

#include "../../def.h"

#include "FS.h"
#include <SPIFFS.h>

#define DISKFS SPIFFS

#define LOGFILE_SUFFIX  ".%02d"
#define LOG_REC_SIZE(sz)    (sz+4)

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

#define logRCount(fname, type)      (logSize(fname)/LOG_REC_SIZE(sizeof(type)))
#define logRCountFull(fname, type)  (logSizeFull(fname)/LOG_REC_SIZE(sizeof(type)))

bool logRotate(const char *_fname, uint8_t count);

/* ------------------------------------------------------------------------------------------- *
 *  Базовый ввод/вывод
 * ------------------------------------------------------------------------------------------ */
class File;
bool fread(File &fh, uint8_t *data, uint16_t dsz, bool tailnull = true);

template <typename T>
bool fread(File &fh, T &data, bool tailnull = true) {
    return fread(fh, reinterpret_cast<uint8_t *>(&data), sizeof(T), tailnull);
}

bool fwrite(File &fh, const uint8_t *data, uint16_t dsz);

template <typename T>
bool fwrite(File &fh, const T &data) {
    return fwrite(fh, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
}


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
        logRead(reinterpret_cast<uint8_t *>(&data), sizeof(T), _fname, index);
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение всех записей, начиная с индекса ibeg (при ibeg=0 - самая первая запись),
 *  Возвращает индекс следующей позиции после крайней прочитанной
 * ------------------------------------------------------------------------------------------- */
int32_t logFileRead(
            bool (*hndhead)(const uint8_t *data), uint16_t dhsz, 
            bool (*hnditem)(const uint8_t *data), uint16_t disz, 
            const char *_fname, uint16_t fnum = 1, size_t ibeg = 0
        );


template <typename Th, typename Ti>
int32_t logFileRead(
            bool (*hndhead)(const Th *data),
            bool (*hnditem)(const Ti *data), 
            const char *_fname, uint16_t fnum = 1, size_t ibeg = 0) {
    return
        logFileRead(
            reinterpret_cast<bool (*)(const uint8_t *data)>(hndhead), sizeof(Th), 
            reinterpret_cast<bool (*)(const uint8_t *data)>(hnditem), sizeof(Ti), 
            _fname, fnum, ibeg
        );
}
template <typename Ti>
int32_t logFileReadMono(
            bool (*hnditem)(const Ti *data), 
            const char *_fname, uint16_t fnum = 1, size_t ibeg = 0) {
    return
        logFileRead(
            NULL, 0, 
            reinterpret_cast<bool (*)(const uint8_t *data)>(hnditem), sizeof(Ti), 
            _fname, fnum, ibeg
        );
}


/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма файла
 * ------------------------------------------------------------------------------------------- */
typedef struct  __attribute__((__packed__)) logchs_s {
    uint16_t    csa;
    uint16_t    csb;
    uint32_t    sz;
    
    bool operator== (const struct logchs_s & cks) {
        return (this == &cks) || ((this->csa == cks.csa) && (this->csb == cks.csb) && (this->sz == cks.sz));
    };
    operator bool() { return (csa != 0) && (csb != 0) && (sz != 0); }
} logchs_t;

logchs_t logChkSumFull(size_t dsz, const char *_fname, uint8_t num = 1);
uint32_t logChkSumBeg(size_t dsz, const char *_fname, uint8_t num = 1);

uint8_t logFind(const char *_fname, size_t dsz, const logchs_t &cks);
uint8_t logFind(const char *_fname, size_t dsz, const uint32_t &cks);


/* ------------------------------------------------------------------------------------------- *
 *  Ренумерация файлов
 * ------------------------------------------------------------------------------------------- */
bool logRenum(const char *_fname);

/* ------------------------------------------------------------------------------------------- *
 *  Удаление файла
 * ------------------------------------------------------------------------------------------- */
int logRemoveLast(const char *_fname, bool removeFirst = false);
int logRemoveAll(const char *_fname, bool removeFirst = false);

#endif // _file_log_H
