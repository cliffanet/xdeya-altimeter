
#include "filetxt.h"
#include <string.h>
#include "../log.h"

#define LINEEND         '\n'
#define ISLINEEND(c)    ((c == '\r') || (c == '\n'))
#define ISSPACE(c)      ((c == ' ') || (c == '\t'))

/* ------------------------------------------------------------------------------------------- *
 *  FileTxt - работа с текстовыми файлами
 * ------------------------------------------------------------------------------------------- */

int FileTxt::read_param(char *str, size_t sz) {
    int len = 0;
    uint8_t c = '\0';
    
    while (available() > 0) {
        c = read();
        if (ISLINEEND(c)) {
            seekback();
            break;
        }
        if (ISSPACE(c))
            break;
        if ((str != NULL) && (sz > 1)) {
            *str = c;
            str++;
            sz --;
        }
        
        len++;
    }
    
    if (ISSPACE(c)) {
        // стравливаем все пробелы после параметра
        while (available() > 0) {
            c = read();
            if (!ISSPACE(c) || ISLINEEND(c)) {
                // Возвращаем обратно "не пробел" или переход на след строку
                seekback();
                break;
            }
        }   
    }
    
    if ((str != NULL) && (sz > 0))
        *str = '\0';
    
    return len;
}

int FileTxt::read_line(char *str, size_t sz) {
    int len = 0;
    uint8_t c = '\0';
    
    while (available() > 0) {
        c = read();
        if (ISLINEEND(c))
            break;
        
        if ((str != NULL) && (sz > 1)) {
            *str = c;
            str++;
            sz --;
        }
        
        len++;
    }
    
    while ((available() > 0) && (c != LINEEND))
        // стравливаем все варианты окончания строки, пока не наткнёмся на '\n'
        c = read();
    
    if ((str != NULL) && (sz > 0))
        *str = '\0';
    
    return len;
}

bool FileTxt::find_param(const char *name_P) {
    size_t len = strlen_P(name_P);
    if (len < 1)
        return false;
    
    char str[len+1];
    char name[len+1];
    
    strcpy_P(name, name_P);
    
    while (available() > 0) {
        read_param(str, sizeof(str));
        if (strcmp(str, name) == 0)
            return true;
        read_line(NULL, 0);
    }
    
    return false;
}

bool FileTxt::print_line(const char *str) {
    size_t len = strlen(str);
    char s[len+1];
    
    strcpy(s, str);
    s[len] = LINEEND;
    
    return write(reinterpret_cast<uint8_t *>(s), len+1) == len+1;
}

bool FileTxt::print_param(const char *name_P) {
    size_t len = strlen_P(name_P);
    char s[len+1];
    
    strcpy_P(s, name_P);
    s[len] = ' ';
    
    return write(reinterpret_cast<uint8_t *>(s), len+1) == len+1;
}

bool FileTxt::print_param(const char *name_P, const char *str) {
    size_t nlen = strlen_P(name_P);
    size_t slen = strlen(str);
    char s[nlen + slen + 2];
    
    strcpy_P(s, name_P);
    s[nlen] = ' ';
    strcpy(s+nlen+1, str);
    s[nlen+1+slen] = LINEEND;
    
    return write(reinterpret_cast<uint8_t *>(s), nlen+1+slen+1) == nlen+1+slen+1;
}

uint32_t FileTxt::chksum() {
    cks8_t cks;
    
    cks.clear();
    uint8_t buf[256];
    
    while (available() > 0) {
        auto sz = read(buf, sizeof(buf));
        CONSOLE("read: %d", sz);
        if (sz <= 0)
            return 0;
        
        cks.add(buf, sz);
    }
    
    return cks.full();
}

size_t FileTxt::line_count() {
    size_t count = available() > 0 ? 1 : 0;
    uint8_t buf[256];
    
    while (available() > 0) {
        auto sz = read(buf, sizeof(buf));
        if (sz <= 0)
            return 0;
        
        for (auto &d : buf) {
            sz --;
            if ((d == LINEEND) && ((sz > 0) || (available() > 0)))
                count++;
            
            if (sz < 1)
                break;
        }
    }
    
    CONSOLE("count: %d", count);
    
    return count;
}

bool FileTxt::seek2line(size_t num) {
    CONSOLE("find line #%d", num);
    
    while ((num > 0) && (available() > 0)) {
        uint8_t buf[256];
        auto sz = read(buf, sizeof(buf));
        if (sz <= 0)
            return 0;
        
        for (auto &d : buf) {
            sz --;
            if (d == LINEEND)
                num--;
            
            if ((sz < 1) || (num < 1))
                break;
        }
        
        if ((num == 0) && (sz > 0))
            seekback(sz);
    }
    
    if (num != 0)
        return false;
    
    return true;
}
