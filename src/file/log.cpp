
#include "log.h"
#include "../log.h"
#include "../view/base.h"


/* ------------------------------------------------------------------------------------------- *
 *  Базовые функции по работе с файлами
 * ------------------------------------------------------------------------------------------- */
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

static void fcks(uint8_t c, uint8_t &cka, uint8_t &ckb) {
    cka += c;
    ckb += cka;
}

#define RSZ(sz)     LOG_REC_SIZE(sz)

bool fread(File &fh, uint8_t *data, uint16_t dsz, bool tailnull) {
    uint16_t sz = 0;
    uint8_t bs[2];
    uint8_t cka = 0, ckb = 0;
    
    viewIntDis();
    
    if (fh.read(bs, 2) != 2) {
        CONSOLE("fail read len");
        viewIntEn();
        return false;
    }
    
    viewIntEn();
    
    fcks(bs[0], cka, ckb);
    fcks(bs[1], cka, ckb);
    
    sz |= bs[0];
    sz |= bs[1] << 8;
    sz+=2;
    
    uint8_t buf[sz], *b = buf;
    size_t sz1 = fh.read(buf, sz);
    
    if (sz1 != sz) {
        CONSOLE("fail read to buf: readed=%d of %d", sz1, sz);
        return false;
    }
    
    while ((sz > 2) && (dsz > 0)) {
        fcks(*b, cka, ckb);
        *data = *b;
        b++;
        sz--;
        data++;
        dsz--;
    }
    
    while (sz > 2) {
        fcks(*b, cka, ckb);
        b++;
        sz--;
    }
    
    while (tailnull && (dsz > 0)) {
        *data = 0;
        data++;
        dsz--;
    }
    
    if ((b[0] != cka) || (b[1] != ckb)) {
        CONSOLE("read fail %d+4 bytes; cka: 0x%02x=0x%02x; ckb: 0x%02x=0x%02x", sz1-2, b[0], cka, b[1], ckb);
        return false;
    }

    //CONSOLE("readed %d+4 bytes; cka=0x%02x; ckb=0x%02x", sz1-2, cka, ckb);
    
    return true;
}

bool fwrite(File &fh, const uint8_t *data, uint16_t dsz) {
    uint16_t sz = RSZ(dsz);
    uint8_t buf[sz], *b = buf;
    uint8_t cka = 0, ckb = 0;
    
    *b = dsz & 0xff;
    fcks(*b, cka, ckb);
    b++;
    *b = (dsz & 0xff00) >> 8;
    fcks(*b, cka, ckb);
    b++;
    
    while (dsz > 0) {
        fcks(*data, cka, ckb);
        *b = *data;
        b++;
        data++;
        dsz--;
    }
    
    *b = cka;
    b++;
    *b = ckb;
    
    viewIntDis();
    
    size_t sz1 = fh.write(buf, sz);
    viewIntEn();
    if (sz1 != sz) {
        CONSOLE("write error: saved=%d of %d", sz1, sz);
        return false;
    }
    
    //CONSOLE("save record: sz=%d+4; cka=0x%02x; ckb=0x%02x;", sz-4, cka, ckb);
    
    return true;
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
    
    viewIntDis();
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return -1;
    
    auto sz = fh.size();
    fh.close();
    
    viewIntEn();
        
    return sz;
}

size_t logSizeFull(const char *_fname) {
    uint8_t n = 1;
    char fname[37];
    size_t sz = 0;
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, n);
    
    viewIntDis();
    
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
    
    viewIntEn();
    
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
    
    viewIntDis();

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
            viewIntEn();
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
            viewIntEn();
            return false;
        }
        CONSOLE("file '%s' renamed to '%s'", fname, fname1);
        
        n--;
    }
    
    viewIntEn();

    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Дописывание в конец
 * ------------------------------------------------------------------------------------------- */
bool logAppend(const char *_fname, const uint8_t *data, uint16_t dsz, size_t maxrcnt, uint8_t count) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    
    logFSuffix(fname+flen, 1);
    
    viewIntDis();
    
    if (DISKFS.exists(fname)) {
        File fh = DISKFS.open(fname);
        if (!fh) {
            viewIntEn();
            return false;
        }
        
        auto sz = fh.size();
        fh.close();
        
        if ((sz + RSZ(dsz)) > (maxrcnt * RSZ(dsz)))
            if (!logRotate(_fname, count)) {
                viewIntEn();
                return false;
            }
    }
    
    File fh = DISKFS.open(fname, FILE_APPEND);
    if (!fh) {
        viewIntEn();
        return false;
    }
    
    auto ok = fwrite(fh, data, dsz);
    fh.close();
    
    viewIntEn();
    
    return ok;
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
        
        viewIntDis();
        
        File fh = DISKFS.open(fname);
        if (!fh) {
            viewIntEn();
            return false;
        }
        
        auto sz = fh.size();
        auto count = sz / RSZ(dsz);
        if (count <= index) {
            index -= count;
            fh.close();
            continue;
        }
        
        CONSOLE("index: %d; dsz: %d; read from: %d; fsize: %d", index, dsz, sz - (index * RSZ(dsz)) - RSZ(dsz), fh.size());
        
        fh.seek(sz - (index * RSZ(dsz)) - RSZ(dsz));
        auto ok = fread(fh, data, dsz);
        fh.close();
        viewIntEn();
        
        return ok;
    }
    
    return false;
}

/* ------------------------------------------------------------------------------------------- *
 *  Чтение всех записей, начиная с индекса i:
 *      при i=0 - с первой записи,
 *      при i=1 - со второй, 
 *  если dhsz != 0, то от начала файла будет отступ в dhsz байт, которые ередадутся через hndhead
 *  
 *  Возвращает индекс записи, на которой остановилось чтение, чтобы в след. раз с него начать
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
    
    viewIntDis();
    File fh = DISKFS.open(fname);
    if (!fh) {
        viewIntEn();
        return -1;
    }
    
    CONSOLE("file open: %s (%d) avail: %d/%d/%d/%d", fname, ibeg, fh.size(), fh.available(), dhsz, disz);
    
    if (dhsz > 0) {
        if (fh.available() < RSZ(dhsz)) {
            fh.close();
            viewIntEn();
            return 0;
        }
        
        uint8_t data[dhsz];
        if (!fread(fh, data, dhsz)) {
            fh.close();
            viewIntEn();
            return -1;
        }
        if ((hndhead != NULL) && !hndhead(data)) {
            CONSOLE("head hnd error");
            fh.close();
            viewIntEn();
            return -1;
        }
    }
    
    if (ibeg >= 0) {
        size_t sz = RSZ(disz) * ibeg;
        if (dhsz > 0) sz += RSZ(dhsz);
        
        if (sz >= fh.size()) {
            viewIntEn();
            return (fh.size()-RSZ(dhsz)) / RSZ(disz);
        }
        CONSOLE("seek to: %d", sz);
        fh.seek(sz, SeekSet);
    }
    
    uint8_t data[disz];
    
    while (fh.available() >= RSZ(disz)) {
        if (!fread(fh, data, disz)) {
            fh.close();
            viewIntEn();
            return -1;
        }
        if ((hnditem != NULL) && !hnditem(data)) {
            CONSOLE("item[%d] hnd error");
            fh.close();
            viewIntEn();
            return -1;
        }
        ibeg++;
    }
    
    fh.close();
    viewIntEn();
    
    return ibeg;
}

/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма файла
 * ------------------------------------------------------------------------------------------- */
// Контрольная сумма по участку в начале файла, конце и размеру файла
// Подходит для недополняемых файлов, возвращает logchs_t
logchs_t logChkSumFull(size_t dsz, const char *_fname, uint8_t num) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    logchs_t cks = { 0, 0, 0 };
    uint8_t data[dsz];
    
    logFSuffix(fname+flen, num);
    viewIntDis();
    File fh = DISKFS.open(fname);
    if (!fh) {
        viewIntEn();
        return cks;
    }
    
    if (fh.read(data, dsz) != dsz) {
        fh.close();
        viewIntEn();
        return cks;
    }
    cks.sz = fh.size();
    
    for (size_t i=0; i<dsz; i++) {
        cks.csa += data[i];
        cks.csb += cks.csa;
    }
    
    fh.seek(cks.sz - dsz);
    if (fh.read(data, dsz) != dsz) {
        cks.csa = 0;
        cks.csb = 0;
        cks.sz = 0;
        fh.close();
        viewIntEn();
        return cks;
    }
    
    for (size_t i=0; i<dsz; i++) {
        cks.csa += data[i];
        cks.csb += cks.csa;
    }
    
    fh.close();
    viewIntEn();
    return cks;
}

// Более простая проверка контрольной суммы только по началу файлов.
// Для постоянно дополняемых файлов годится только такой вариант
uint32_t logChkSumBeg(size_t dsz, const char *_fname, uint8_t num) {
    char fname[36];
    const byte flen = logFName(fname, sizeof(fname), _fname);
    uint8_t data[dsz];
    
    logFSuffix(fname+flen, num);
    viewIntDis();
    File fh = DISKFS.open(fname);
    if (!fh) {
        viewIntEn();
        return 0;
    }
    
    if (fh.read(data, dsz) != dsz) {
        fh.close();
        viewIntEn();
        return 0;
    }
    
    uint8_t csa = 0;
    uint8_t csb = 0;
    for (size_t i=0; i<dsz; i++) {
        csa += data[i];
        csb += csa;
    }
    
    fh.close();
    viewIntEn();
    return (csa << 24) | (csb << 16) | (dsz & 0xffff);
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
    
    viewIntDis();
    
    while (nn < 100) {
        logFSuffix(src+flen, nn);
        bool ex = DISKFS.exists(src);
        if (ex) {
            File fh = DISKFS.open(src);
            CONSOLE("Rename preopen: %s - %d", src, fh == true);
            fh.close();
            if (!DISKFS.rename(src, "/tmp")) {
                CONSOLE("Rename fail1: %s -> %s", src, dst);
                viewIntEn();
                return false;
            }
            if (!DISKFS.rename("/tmp", dst)) {
                CONSOLE("Rename fail2: %s -> %s", src, dst);
                viewIntEn();
                return false;
            }
            CONSOLE("Renum OK: %s -> %s", src, dst);
            n++;
            logFSuffix(dst+flen, n);
        }
        nn++;
    }
    
    viewIntEn();
    
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
    viewIntDis();
    if (!DISKFS.remove(fname)) {
        CONSOLE("Can't remove file '%'", fname);
        viewIntEn();
        return -1;
    }
    viewIntEn();
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
    
        viewIntDis();
        
        if (!DISKFS.remove(fname)) {
            CONSOLE("Can't remove file '%s'", fname);
            viewIntEn();
            return -1;
        }
        viewIntEn();
        CONSOLE("file '%s' removed", fname);
    
        n++;
    }
    
    return -1;
}

