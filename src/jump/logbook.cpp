/*
    Jump LogBook
*/

#include "logbook.h"

bool FileLogBook::append(const item_t &item) {
    if (!isvalid() && !open(MODE_APPEND))
        return false;
    
    if ((sizefile()+1) > JMPLOGBOOK_ITEM_COUNT) {
        if (!rotate(JMPLOGBOOK_FILE_COUNT))
            return false;
        if (!open(MODE_APPEND))
            return false;
    }
    
    return add(item);
}
