
#include "log.h"
#include "../log.h"

static const byte logFName(char *fname, size_t sz, const char *_fnamesrc) {
    if (sz <= 7) return 0;
    sz -= 7;
    
    fname[0] = '/';
    strncpy_P(fname+1, _fnamesrc, sz);
    fname[sz] = '\0';
    return strlen(fname);
}
static int logFSuffix(char *fname, uint8_t num) {
    return sprintf_P(fname, PSTR(LOGFILE_SUFFIX), num);
}

/* ------------------------------------------------------------------------------------------- *
 *  Существование файла
 * ------------------------------------------------------------------------------------------- */
bool logExists(const char *_fname, uint8_t num) {
    char fname[37];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, num);
    
    return DISKFS.exists(fname);
}

/* ------------------------------------------------------------------------------------------- *
 *  Количество файлов
 * ------------------------------------------------------------------------------------------- */
int logCount(const char *_fname) {
    int n = 1;
    char fname[37];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    while (n < 255) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname))
            return n - 1;
    
        n++;
    }
    
    return -1;
}

/* ------------------------------------------------------------------------------------------- *
 *  Размер файла
 * ------------------------------------------------------------------------------------------- */
size_t logSize(const char *_fname, uint8_t num) {
    char fname[37];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, num);
    
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
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, n);
    
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
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    strcpy(fname1, fname);
    
    if (count == 0)
        count = 255;

    // ищем первый свободный слот
    while (1) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname)) break;
        if (n < count) {
            n++; // пока ещё не достигли максимального номера файла, продолжаем
            continue;
        }
        // достигнут максимальный номер файла
        // его надо удалить и посчитать первым свободным слотом
        if (!DISKFS.remove(fname)) {
            CONSOLE("Can't remove file '%s'", fname);
            return false;
        }
        CONSOLE("file '%s' removed", fname);
        break;
    }

    // теперь переименовываем все по порядку
    while (n > 1) {
        logFSuffix(fname+flen, n-1);
        logFSuffix(fname1+flen, n);
        
        if (!DISKFS.rename(fname, fname1)) {
            CONSOLE("Can't rename file '%s' to '%s'", fname, fname1);
            return false;
        }
        CONSOLE("file '%s' renamed to '%s'", fname, fname1);
        
        n--;
    }

    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Дописывание в конец
 * ------------------------------------------------------------------------------------------- */
bool logAppend(const char *_fname, const uint8_t *data, uint16_t dsz, size_t maxrcnt, uint8_t count) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, 1);
    
    if (DISKFS.exists(fname)) {
        File fh = DISKFS.open(fname);
        if (!fh) return false;
        
        auto sz = fh.size();
        fh.close();
        
        if ((sz + dsz) > (maxrcnt*dsz))
            if (!logRotate(_fname, count))
                return false;
    }
    
    File fh = DISKFS.open(fname, FILE_APPEND);
    if (!fh)
        return false;
    
    auto sz = fh.write(data, dsz);
    fh.close();
    
    return sz == dsz;
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение записи с индексом i с конца (при i=0 - самая последняя запись, при i=1 - предпоследняя)
 * ------------------------------------------------------------------------------------------- */
bool logRead(uint8_t *data, uint16_t dsz, const char *_fname, size_t index) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    for (uint16_t n = 1; n <= 1000; n++) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname))
            return false;
        
        File fh = DISKFS.open(fname);
        if (!fh) return false;
        
        auto sz = fh.size();
        auto count = sz / dsz;
        if (count <= index) {
            index -= count;
            fh.close();
            continue;
        }
        
        fh.seek(sz - (index * dsz) - dsz);
        sz = fh.read(data, dsz);
        fh.close();
        
        return sz == dsz;
    }
    
    return false;
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение всех записей, начиная с индекса i:
 *      при i=0 - с первой записи,
 *      при i=1 - со второй, 
 *  если dhsz != 0, то от начала файла будет отступ в dhsz байт, которые ередадутся через hndhead
 *  
 *      
 * ------------------------------------------------------------------------------------------- */
int32_t logFileRead(
            bool (*hndhead)(const uint8_t *data), uint16_t dhsz, 
            bool (*hnditem)(const uint8_t *data), uint16_t disz, 
            const char *_fname, uint16_t fnum, size_t ibeg
        ) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, fnum);
    if (!DISKFS.exists(fname))
        return -1;
    
    File fh = DISKFS.open(fname);
    if (!fh) return -1;
    
    CONSOLE("logFileRead open: %s (%d) avail: %d/%d/%d/%d", fname, ibeg, fh.size(), fh.available(), dhsz, disz);
    
    if (dhsz > 0) {
        if (fh.available() < dhsz)
            return 0;
        if (hndhead != NULL) {
            uint8_t data[dhsz];
            auto sz = fh.read(data, dhsz);
            if (
                    (sz != dhsz) ||
                    (data[0] != LOG_MGC1) ||
                    (data[dhsz-1] != LOG_MGC2) ||
                    !hndhead(data)
                ) {
                CONSOLE("logFileRead head err: sz=%d, dhsz=%d, MGC1=0x%02X, MGC2=0x%02X", sz, dhsz, data[0], data[dhsz-1]);
                fh.close();
                return -1;
            }
        }
    }
    
    if (ibeg >= 0) {
        size_t sz = dhsz + (disz * ibeg);
        if (sz >= fh.size())
            return (fh.size()-dhsz) / disz;
        fh.seek(sz, SeekSet);
    }
    
    uint8_t data[disz];
    int32_t cnt = 0;
    
    while (fh.available() >= disz) {
        auto sz = fh.read(data, disz);
        if (
                (sz != disz) ||
                (data[0] != LOG_MGC1) ||
                (data[disz-1] != LOG_MGC2) ||
                !hnditem(data)
            ) {
            CONSOLE("logFileRead item err: [%d] sz=%d, dsz=%d, MGC1=0x%02X, MGC2=0x%02X", ibeg, sz, disz, data[0], data[disz-1]);
            fh.close();
            return -1;
        }
        cnt++;
    }
    
    fh.close();
    
    return cnt;
}

/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма файла
 * ------------------------------------------------------------------------------------------- */
// Контрольная сумма по участку в начале файла, конце и размеру файла
// Подходит для недополняемых файлов, возвращает logchs_t
logchs_t logChkSumFull(size_t dsz, const char *_fname, uint8_t num) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    logchs_t cks = { 0, 0 };
    uint8_t data[dsz];
    
    logFSuffix(fname+flen, num);
    File fh = DISKFS.open(fname);
    if (!fh) return cks;
    
    if (fh.read(data, dsz) != dsz) {
        fh.close();
        return cks;
    }
    cks.sz = fh.size();
    
    for (size_t i=0; i<dsz; i++)
        cks.cs += data[i];
    
    fh.seek(cks.sz - dsz);
    if (fh.read(data, dsz) != dsz) {
        cks.cs = 0;
        cks.sz = 0;
        fh.close();
        return cks;
    }
    
    for (size_t i=0; i<dsz; i++)
        cks.cs += data[i];
    
    fh.close();
    return cks;
}

// Более простая проверка контрольной суммы только по началу файлов.
// Для постоянно дополняемых файлов годится только такой вариант
uint32_t logChkSumBeg(size_t dsz, const char *_fname, uint8_t num) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    uint8_t data[dsz];
    
    logFSuffix(fname+flen, num);
    File fh = DISKFS.open(fname);
    if (!fh) return 0;
    
    if (fh.read(data, dsz) != dsz) {
        fh.close();
        return 0;
    }
    
    uint16_t cs = 0;
    for (size_t i=0; i<dsz; i++)
        cs += data[i];
    
    fh.close();
    return (cs << 16) | (dsz & 0xff);
}

uint8_t logFind(const char *_fname, size_t dsz, const logchs_t &cks) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    for (uint8_t n = 1; n <= 99; n++) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname))
            break;
        
        auto cks1 = logChkSumFull(dsz, _fname, n);
        if (cks1 == cks)
            return n;
    }
    
    return 0;
}

uint8_t logFind(const char *_fname, size_t dsz, const uint32_t &cks) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    for (uint8_t n = 1; n <= 99; n++) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname))
            break;
        
        auto cks1 = logChkSumBeg(dsz, _fname, n);
        if (cks1 == cks)
            return n;
    }
    
    return 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  Ренумерация файлов
 * ------------------------------------------------------------------------------------------- */
bool logRenum(const char *_fname) {
    int n = 1, nn;
    char dst[37], src[37];
    const byte flen = logFName(dst, sizeof(dst), _fname);
    strcpy(src, dst);

    // сначала ищем свободный слот dst
    while (n < 99) {
        logFSuffix(dst+flen, n);
        if (!DISKFS.exists(dst)) break;
        CONSOLE("Renuming find, exists: %s", dst);
        n++;
    }
    
    // теперь ищем пробел и первый занятый номер - src
    if (nn <= n) nn = n+1;
    nn=n+1;
    
    while (nn < 100) {
        logFSuffix(src+flen, nn);
        bool ex = DISKFS.exists(src);
        if (ex) {
            File fh = DISKFS.open(src);
            CONSOLE("Rename preopen: %s - %d", src, fh == true);
            fh.close();
            if (!DISKFS.rename(src, "/tmp")) {
                CONSOLE("Rename fail1: %s -> %s", src, dst);
                return false;
            }
            if (!DISKFS.rename("/tmp", dst)) {
                CONSOLE("Rename fail2: %s -> %s", src, dst);
                return false;
            }
            CONSOLE("Renum OK: %s -> %s", src, dst);
            n++;
            logFSuffix(dst+flen, n);
        }
        nn++;
    }
    
    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Удаление самого старшего файла, возвращает номер удалённого файла
 * ------------------------------------------------------------------------------------------- */
int logRemoveLast(const char *_fname, bool removeFirst) {
    int n = 1;
    char fname[37];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    // ищем первый свободный слот
    while (n < 255) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname)) break;
        n++;
    }
    if (n <= 1)
        return -1;
    
    n--;
    if ((n <= 1) && !removeFirst)
        return -1;
    
    logFSuffix(fname+flen, n);
    if (!DISKFS.remove(fname)) {
        CONSOLE("Can't remove file '%'", fname);
        return -1;
    }
    CONSOLE("file '%s' removed", fname);
    
    return n;
}

/* ------------------------------------------------------------------------------------------- *
 *  Удаление всех файлов, возвращает сколько файлов удалено
 * ------------------------------------------------------------------------------------------- */
int logRemoveAll(const char *_fname, bool removeFirst) {
    int n = removeFirst ? 1 : 2;
    char fname[37];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    while (n < 255) {
        logFSuffix(fname+flen, n);
        if (!DISKFS.exists(fname))
            return n - (removeFirst ? 1 : 2);
        
        if (!DISKFS.remove(fname)) {
            CONSOLE("Can't remove file '%s'", fname);
            return -1;
        }
        CONSOLE("file '%s' removed", fname);
    
        n++;
    }
    
    return -1;
}

