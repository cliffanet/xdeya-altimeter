
#include "../../def.h"
#include "file.h"
#include "../log.h"

#include <FS.h>
#include <SPIFFS.h>

#ifdef USE_SDCARD
#include <SD.h>
#endif

/* ------------------------------------------------------------------------------------------- *
 *  Стандартная обвязка с файлами
 * ------------------------------------------------------------------------------------------- */
static uint8_t initok = 0;

#ifdef USE_SDCARD
bool fileExtInit() {
    if (!SD.begin(PIN_SDCARD)) {
        CONSOLE("SD Mount Failed");
        initok &= ~1;
        return false;
    }
    
    CONSOLE("card init: type: %d, size: %llu", SD.cardType(), SD.cardSize());
    
    initok |= 1;
    
    return true;
}

void fileExtStop() {
    SD.end();
    CONSOLE("card stop");
    initok &= ~1;
}
#endif

bool fileIntInit() {
    if (!SPIFFS.begin(true)) {
        CONSOLE("SPIFFS Mount Failed");
        initok &= ~2;
        return false;
    }
    
    initok |= 2;
    
    return true;
}

void fileIntStop() {
    SPIFFS.end();
    initok &= ~2;
}

bool file_exists(const char *fname, bool external) {
#ifdef USE_SDCARD
    if (external && ((initok & 1) > 0))
        return SD.exists(fname);
    else
#endif
    if ((initok & 2) > 0)
        return SPIFFS.exists(fname);
    
    return false;
}

bool file_remove(const char *fname, bool external) {
#ifdef USE_SDCARD
    if (external && ((initok & 1) > 0))
        return SD.remove(fname);
    else
#endif
    if ((initok & 2) > 0)
        return SPIFFS.remove(fname);
    
    return false;
}

bool file_rename(const char *fname1, const char *fname2, bool external) {
#ifdef USE_SDCARD
    if (external && ((initok & 1) > 0))
        return SD.rename(fname1, fname2);
    else
#endif
    if ((initok & 2) > 0)
        return SPIFFS.rename(fname1, fname2);
    
    return false;
}

static File file_open(const char *fname, FileMy::mode_t mode, bool external) {
    CONSOLE("fname: %c(%d) / %s", external ? 'e' : 'i', initok, fname);

#ifdef USE_SDCARD
    if (external && ((initok & 1) > 0))
        switch (mode) {
            case FileMy::MODE_READ:
                return SD.open(fname, FILE_READ);
                
            case FileMy::MODE_WRITE:
                return SD.open(fname, FILE_WRITE);
                
            case FileMy::MODE_APPEND:
                return SD.open(fname, FILE_APPEND);
        }
    else
#endif
    if ((initok & 2) > 0)
        switch (mode) {
            case FileMy::MODE_READ:
                return SPIFFS.open(fname, FILE_READ);
                
            case FileMy::MODE_WRITE:
                return SPIFFS.open(fname, FILE_WRITE);
                
            case FileMy::MODE_APPEND:
                return SPIFFS.open(fname, FILE_APPEND);
        };
    
    return File();
}

/* ------------------------------------------------------------------------------------------- */

void fileName(char *fname, size_t sz, const char *fname_P, uint8_t num) {
    if (sz < 2)
        return;
    *fname = '/';
    strncpy_P(fname+1, fname_P, sz-1);
    fname[sz-1] = '\0';
    
    if (num > 0) {
        size_t len = strlen(fname);
        if (sz > len)
            snprintf_P(fname + len, sz - len, PSTR(FILE_NUM_SUFFIX), num);
    }
}

bool fileExists(const char *fname_P, uint8_t num, bool external) {
    char fname[30];
    fileName(fname, sizeof(fname), fname_P, num);
    
    return file_exists(fname, external);
}

bool fileRemove(const char *fname_P, uint8_t num, bool external) {
    char fname[30];
    fileName(fname, sizeof(fname), fname_P, num);
    
    return file_remove(fname, external);
}

size_t fileCount(const char *fname_P, bool external) {
    int n = 1;
    char fname[37];
    
    while (n < 99) {
        fileName(fname, sizeof(fname), fname_P, n);
        if (!file_exists(fname, external))
            return n-1;
        n++;
    }
    
    return -1;
}

/* ------------------------------------------------------------------------------------------- *
 *  Свободное место
 * ------------------------------------------------------------------------------------------- */
size_t fileSizeAvail(bool external) {
#ifdef USE_SDCARD
    if (external && ((initok & 1) > 0))
        return SD.usedBytes();
    else
#endif
    if ((initok & 2) > 0)
        return SPIFFS.usedBytes();
    
    return 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  Ренумерация файлов
 * ------------------------------------------------------------------------------------------- */
bool fileRenum(const char *fname_P, bool external) {
    int nd = 1, ns;
    char dst[37], src[37];
    
    // сначала ищем свободный слот dst
    while (nd < 99) {
        fileName(dst, sizeof(dst), fname_P, nd);
        if (!file_exists(dst, external)) break;
        CONSOLE("Renuming find, exists: %s", dst);
        nd++;
    }
    
    // теперь ищем пробел и первый занятый номер - src
    ns=nd+1;
    
    while (ns < 30) {
        fileName(src, sizeof(src), fname_P, ns);
        fileName(dst, sizeof(dst), fname_P, nd);
        if (file_exists(src, external)) {
            //File fh = file_open(src, FileMy::MODE_READ, external);
            //CONSOLE("Rename preopen: %s - %d", src, fh == true);
            //fh.close();
            if (!file_rename(src, dst, external)) {
                CONSOLE("Rename fail: %s -> %s", src, dst);
                return false;
            }
            /*
            if (!file_rename("/tmp", dst, external)) {
                CONSOLE("Rename fail2: %s -> %s", src, dst);
                return false;
            }
            */
            CONSOLE("Renum OK: %s -> %s", src, dst);
            nd++;
        }
        ns++;
    }
    
    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Ротирование файлов
 * ------------------------------------------------------------------------------------------- */
bool fileRotate(const char *fname_P, uint8_t count, bool external) {
    uint8_t n = 1;
    char fname[37], fname1[37];
    
    if (count == 0)
        count = 255;

    // ищем первый свободный слот
    while (1) {
        fileName(fname, sizeof(fname), fname_P, n);
        if (!file_exists(fname, external)) break;
        if (n < count) {
            n++; // пока ещё не достигли максимального номера файла, продолжаем
            continue;
        }
        // достигнут максимальный номер файла
        // его надо удалить и посчитать первым свободным слотом
        if (!file_remove(fname, external)) {
            CONSOLE("Can't remove file '%s'", fname);
            return false;
        }
        CONSOLE("file '%s' removed", fname);
        break;
    }

    // теперь переименовываем все по порядку
    while (n > 1) {
        fileName(fname, sizeof(fname), fname_P, n-1);
        fileName(fname1, sizeof(fname1), fname_P, n);
        
        if (!file_rename(fname, fname1, external)) {
            CONSOLE("Can't rename file '%s' to '%s'", fname, fname1);
            return false;
        }
        CONSOLE("file '%s' renamed to '%s'", fname, fname1);
        
        n--;
    }

    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  FileMy - базовый класс для файлов с множественными чтением/записью
 * ------------------------------------------------------------------------------------------- */
FileMy::FileMy(const FileMy &f) {
    *this = f;
}

FileMy::FileMy(const char *fname_P, mode_t mode, bool external) :
    FileMy()
{
    open_P(fname_P, mode, external);
}

bool FileMy::open(const char *fname, mode_t mode, bool external) {
    fh = file_open(fname, mode, external);
    if (!fh || fh.isDirectory())
        return false;
    
    return true;
}

bool FileMy::open_P(const char *fname_P, mode_t mode, bool external) {
    char fname[30];
    fileName(fname, sizeof(fname), fname_P);
    
    return open(fname, mode, external);
}

bool FileMy::close() {
    if (!fh)
        return false;
    
    CONSOLE("file close: pos=%lu, sz=%lu", fh.position(), fh.size());
    
    fh.close();
    
    return true;
}
