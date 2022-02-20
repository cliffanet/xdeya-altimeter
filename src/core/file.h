/*
    files base functions
*/

#ifndef _core_file_H
#define _core_file_H

#include <stdint.h>
#include "cks.h"

/* ------------------------------------------------------------------------------------------- *
 *  Стандартная обвязка с файлами
 * ------------------------------------------------------------------------------------------- */
void fileName(char *fname, size_t sz, const char *fname_P, bool external = false);
bool fileExists(const char *fname_P, bool external = false);
bool fileRemove(const char *fname_P, bool external = false);

void fileProcess();

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
        
        FileMy();
        FileMy(const FileMy &f);
        FileMy(const char *fname_P, mode_t mode = MODE_READ, bool external = false);
        bool open(const char *fname_P, mode_t mode = MODE_READ, bool external = false);
        bool close();
        bool isvalid();
        size_t available() const;
        uint8_t read();
        bool seekback(size_t sz = 1);
        size_t read(uint8_t *data, size_t sz);
        size_t write(const uint8_t *data, size_t sz);
        size_t size() const;
        
        operator bool() { return isvalid(); }
    
    private:
        uint8_t m_num;
        uint32_t m_id;
};

#endif // _core_file_H
