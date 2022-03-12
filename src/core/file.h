/*
    files base functions
*/

#ifndef _core_file_H
#define _core_file_H

#include <stdint.h>
#include "cks.h"
#include "FS.h"

/* ------------------------------------------------------------------------------------------- *
 *  Стандартная обвязка с файлами
 * ------------------------------------------------------------------------------------------- */
#define FILE_NUM_SUFFIX     ".%02d"

void fileName(char *fname, size_t sz, const char *fname_P, uint8_t num = 0);
bool fileExists(const char *fname_P, bool external = false);
bool fileRemove(const char *fname_P, bool external = false);

size_t fileCount(const char *fname_P, bool external = false);
bool fileRenum(const char *fname_P, bool external = false);
bool fileRotate(const char *fname_P, uint8_t count, bool external = false);

/* ------------------------------------------------------------------------------------------- *
 *  FileMy - базовый класс для файлов с множественными чтением/записью
 * ------------------------------------------------------------------------------------------- */
class FileMy {
    public:
        
        typedef enum {
            MODE_READ,
            MODE_WRITE,
            MODE_APPEND
        } mode_t;
        
        FileMy() {}
        FileMy(const FileMy &f);
        FileMy(const char *fname_P, mode_t mode = MODE_READ, bool external = false);
        bool open(const char *fname, mode_t mode = MODE_READ, bool external = false);
        bool open_P(const char *fname_P, mode_t mode = MODE_READ, bool external = false);
        bool close();
        
        const char * name() const { return fh.name(); }
        size_t available() { return fh.available(); };
        
        operator bool() { return fh && !fh.isDirectory(); }
    
    protected:
        File fh;
};

#endif // _core_file_H
