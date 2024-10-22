/*
    Jump LogBook
*/

#ifndef _jump_logbook_H
#define _jump_logbook_H

#include <pgmspace.h>   // PSTR()

#include "../core/filebin.h"
#include "proc.h"

// имя файла для хранения простых логов
#define JMPLOGBOOK_NAME          "logsimple"
// Сколько прыжков в одном файле
#define JMPLOGBOOK_ITEM_COUNT    50
// Сколько файлов прыгов максимум
#define JMPLOGBOOK_FILE_COUNT    5

class FileLogBook : public FileBinNum {
    public:
        // Один прыг в логбуке, где запоминаются только данные на начало, середину и самое окончание прыга
        typedef struct __attribute__((__packed__)) {
            uint32_t    num;
            uint32_t    key;
            tm_t        tm;
            log_item_t  toff;
            log_item_t  beg;
            log_item_t  cnp;
            log_item_t  end;
        } item_t;
        
        FileLogBook() : FileBinNum(PSTR(JMPLOGBOOK_NAME)) {}

        inline size_t sizeitem() const { return sizeof(item_t)+4; }
        size_t pos() const { return fh.position() / sizeitem(); }
        size_t avail() { return fh.available() / sizeitem(); }
        size_t sizefile() const { return fh.size() / sizeitem(); }
        size_t sizeall();
        
        bool getfull(item_t &item, size_t index);
        
        uint32_t chksum();
        uint32_t chksum(uint8_t n);
        uint8_t findfile(uint32_t cks);
        
        bool append(const item_t &item);
        bool seekto(size_t index);
};

#endif // _jump_logbook_H
