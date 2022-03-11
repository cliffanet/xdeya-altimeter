/*
    Jump LogBook
*/

#include "logbook.h"
#include "../log.h"

size_t FileLogBook::sizeall() {
    if (fh)
        fh.close();
    
    size_t sz = 0;
    for (uint8_t n = 1; n < 99; n++) {
        if (!open(n))
            break;
        sz += sizefile();
        fh.close();
    }
    
    return sz;
}

bool FileLogBook::getfull(item_t &item, size_t index) {
    if (fh)
        fh.close();

    for (uint8_t n = 1; n < 99; n++) {
        if (!open(n))
            return false;
        
        auto sz = sizefile();
        if (index >= sz) {
            index -= sz;
            fh.close();
            continue;
        }
        
        size_t pos = (sz-index-1) * sizeitem();
        
        CONSOLE("index: %d; dsz: %d; read from: %d; fsize: %d", index, sizeitem(), pos, fh.size());
        
        fh.seek(pos);
        if (!get(item)) {
            fh.close();
            return false;
        }
        
        fh.close();
        
        return true;
    }
    
    return false;
}

bool FileLogBook::append(const item_t &item) {
    if (!fh && !open(MODE_APPEND))
        return false;
    
    if ((sizefile()+1) > JMPLOGBOOK_ITEM_COUNT) {
        if (!rotate(JMPLOGBOOK_FILE_COUNT))
            return false;
        if (!open(MODE_APPEND))
            return false;
    }
    
    return add(item);
}
