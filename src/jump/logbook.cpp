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

uint32_t FileLogBook::chksum() {
    if (!fh)
        return 0;
    
    fh.seek(0, SeekSet);
    
    uint8_t data[sizeitem()];
    if (fh.read(data, sizeof(data)) != sizeof(data))
        return 0;
    
    uint8_t csa = 0;
    uint8_t csb = 0;
    for (const auto &d : data) {
        csa += d;
        csb += csa;
    }
    
    return (csa << 24) | (csb << 16) | (sizeof(data) & 0xffff);
}

uint32_t FileLogBook::chksum(uint8_t n) {
    if (fh)
        fh.close();
    
    if (!open(n))
        return 0;
    
    uint32_t cks = chksum();
    
    fh.close();
    
    return cks;
}

uint8_t FileLogBook::findfile(uint32_t cks) {
    for (uint8_t n = 1; n < 99; n++) {
        uint32_t cks1 = chksum(n);
        if (cks1 == 0)
            return 0;
        if (cks1 == cks)
            return n;
    }
    
    return 0;
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

bool FileLogBook::seekto(size_t index) {
    return fh.seek( index * sizeitem(), SeekSet );
}
