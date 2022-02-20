
#include "file.h"
#include "../log.h"

#include "FS.h"
#include <SPIFFS.h>

/* ------------------------------------------------------------------------------------------- *
 *  Стандартная обвязка с файлами
 * ------------------------------------------------------------------------------------------- */
static bool file_exists(const char *fname, bool external = false) {
    if (external)
        return file_exists(fname, false);
    
    switch (external) {
        case false:
            return SPIFFS.exists(fname);
    }
    
    return false;
}

static bool file_remove(const char *fname, bool external = false) {
    if (external)
        return file_exists(fname, false);
    
    switch (external) {
        case false:
            return SPIFFS.remove(fname);
    }
    
    return false;
}

static File file_open(const char *fname, FileMy::mode_t mode, bool external) {
    CONSOLE("fname: %s", fname);
    if (external)
        return file_open(fname, mode, false);
    
    switch (external) {
        case false:
            switch (mode) {
                case FileMy::MODE_READ:
                    return SPIFFS.open(fname, FILE_READ);
                    
                case FileMy::MODE_WRITE:
                    return SPIFFS.open(fname, FILE_WRITE);
                    
                case FileMy::MODE_APPEND:
                    return SPIFFS.open(fname, FILE_APPEND);
            }
            break;
    }
    
    return File();
}

static File file_open_P(const char *fname_P, FileMy::mode_t mode, bool external) {
    char fname[30];
    fname[0] = '/';
    strncpy_P(fname+1, fname_P, sizeof(fname)-1);
    fname[sizeof(fname)-1] = '\0';
    
    return file_open(fname, mode, external);
}

void fileName(char *fname, size_t sz, const char *fname_P, bool external) {
    if (sz < 2)
        return;
    *fname = '/';
    strncpy_P(fname+1, fname_P, sz-1);
    fname[sz-1] = '\0';
}

bool fileExists(const char *fname_P, bool external) {
    char fname[30];
    fileName(fname, sizeof(fname), fname_P);
    
    return file_exists(fname);
}

bool fileRemove(const char *fname_P, bool external) {
    char fname[30];
    fileName(fname, sizeof(fname), fname_P);
    
    return file_remove(fname);
}

/* ------------------------------------------------------------------------------------------- *
 *  локальный список хендлеров
 * ------------------------------------------------------------------------------------------- */
typedef struct {
    File fh;
    uint32_t id = 0;
    int16_t io = 0;
} file_hnd_t;

// таймаут в количествах тиков (запусков fileProcess())
#define FILE_IO_TIMEOUT 300
static uint32_t globid = 0;
static file_hnd_t fhall[10];

static uint8_t fhnew() {
    uint8_t num = 0;
    for (auto f: fhall) {
        num ++;
        if (f.id > 0)
            continue;
        return num;
    }
    return 0;
}

static file_hnd_t *fhget(uint8_t num, uint32_t id) {
    if ((num < 1) || (num > sizeof(fhall) / sizeof(file_hnd_t)) || (id == 0))
        return NULL;
    auto &f = fhall[num-1];
    if (f.id != id)
        return NULL;
    
    return &f;
}

void fileProcess() {
    for (auto &f : fhall) {
        if (f.id == 0)
            continue;
        
        if (f.io > 0)
            f.io --;
        if (f.io > 0)
            continue;
        
        CONSOLE("file[globid=%d] timeout: %s", f.id, f.fh.name());
        f.fh.close();
        f.id = 0;
        f.io = 0;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  FileMy - базовый класс для файлов с множественными чтением/записью
 * ------------------------------------------------------------------------------------------- */
FileMy::FileMy() :
    m_num(0),
    m_id(0)
{
    
}

FileMy::FileMy(const FileMy &f) {
    *this = f;
}

FileMy::FileMy(const char *fname_P, mode_t mode, bool external) :
    FileMy()
{
    open(fname_P, mode, external);
}

bool FileMy::open(const char *fname_P, mode_t mode, bool external) {
    if (m_num > 0)
        close();
    
    m_num = fhnew();
    if (m_num == 0)
        return false;
    
    auto &f = fhall[m_num-1];
    f.fh = file_open_P(fname_P, mode, external);
    if (!f.fh) {
        m_num = 0;
        return false;
    }
    
    globid ++;
    m_id = globid;
    f.id = globid;
    f.io = FILE_IO_TIMEOUT;
    
    CONSOLE("num: %d, globid: %d, mode: %d, ok: %d", m_num, globid, mode, f.fh ? 1 : 0);
    
    return true;
}

bool FileMy::close() {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return false;
    
    f->fh.close();
    f->id = 0;
    f->io = 0;
    return true;
}

bool FileMy::isvalid() {
    return fhget(m_num, m_id) != NULL;
}

size_t FileMy::available() const {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return -1;
    
    return f->fh.available();
}

uint8_t FileMy::read() {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return 0;
    
    f->io = FILE_IO_TIMEOUT;
    
    return f->fh.read();
}

bool FileMy::seekback(size_t sz) {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return false;
    
    f->io = FILE_IO_TIMEOUT;
    
    size_t pos = f->fh.position();
    if (sz > pos)
        sz = pos;
    pos -= sz;
    return f->fh.seek(pos, SeekSet);
}

size_t FileMy::read(uint8_t *data, size_t sz) {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return -1;
    
    f->io = FILE_IO_TIMEOUT;
    
    return f->fh.read(data, sz);
}

size_t FileMy::write(const uint8_t *data, size_t sz) {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return -1;
    
    f->io = FILE_IO_TIMEOUT;
    
    return f->fh.write(data, sz);
}

size_t FileMy::size() const {
    auto *f = fhget(m_num, m_id);
    if (f == NULL)
        return -1;
    
    return f->fh.size();
}
