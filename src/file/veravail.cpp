
#include "log.h"
#include "veravail.h"
#include "../log.h"

#define FNAMEINIT   \
    char fname[20]; \
    fname[0] = '/'; \
    strcpy_P(fname+1, PSTR(VERAVAIL_FILE));

/* ------------------------------------------------------------------------------------------- *
 *  Удаление файла
 * ------------------------------------------------------------------------------------------- */
bool verAvailClear() {
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return true;
    return DISKFS.remove(fname);
}

/* ------------------------------------------------------------------------------------------- *
 *  Добавление доступной версии в файл
 * ------------------------------------------------------------------------------------------- */
bool verAvailAdd(const char *ver) {
    FNAMEINIT
    
    File fh = DISKFS.open(fname, FILE_APPEND);
    if (!fh)
        return false;
    
    CONSOLE("save veravail: %s", ver);
    
    fh.print(ver);
    // тут важно, чтобы был определённый строковый разделитель
    // во-первых, так будет легче искать построчно
    // во-вторых, котрольная сумма этого файла будет считаться и на сервере тоже, она должна совпадать
    fh.print('\n');
    fh.close();
    
    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма
 * ------------------------------------------------------------------------------------------- */
uint32_t verAvailChkSum() {
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return 0;
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return 0;
    
    uint16_t csz = fh.size();
    uint8_t csa = 0;
    uint8_t csb = 0;
    uint8_t buf[256];
    
    while (fh.available() > 0) {
        auto len = fh.read(buf, sizeof(buf));
        if (len <= 0) {
            fh.close();
            return 0;
        }
        
        for (int i=0; i<len; i++) {
            csa += buf[i];
            csb += csa;
        }
    }
    
    fh.close();
    
    return (csa << 24) | (csb << 16) | csz;
}

/* ------------------------------------------------------------------------------------------- *
 *  Поиск доступной версии по номеру в файле
 * ------------------------------------------------------------------------------------------- */
bool verAvailGet(int num, char *ver) {
    if (num <= 0) // num должен начинаться с 1
        return false;
    
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return false;
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return false;
    
    CONSOLE("verAvailGet search: %d", num);
    
    bool founded = false;
    
    while (fh.available() > 0) {
        char s = fh.read(); // читаем до окончания строки
        if (s == '\n') {
            if (founded) { // нужная строка закончилась
                break;
            }
            else {
                num --;
                if (num <= 0) {
                    // мы нашли нужную строчку, но есть ли у неё продолжение?
                    founded = fh.available();
                    if (ver == NULL)    // если мы не указали ver, то нам и не надо дальше,
                        break;          // только сам факт, что нужная строка в файле есть
                }
            }
        }
        else
        if (founded && (ver != NULL))
            *ver = s;
    }
        
    if (ver != NULL)
        *ver = '\0';
    
    fh.close();
    
    if (founded)
        CONSOLE("verAvailGet founded");
    else
        CONSOLE("verAvailGet not found");
    
    return founded;
}

int verAvailCount() {
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return false;
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return false;
    
    int cnt = 0;
    bool newstr = true;
    while (fh.available() > 0) {
        char s = fh.read(); // читаем до окончания строки
        if (newstr) {
            cnt++;
            newstr = false;
        }
        if (s == '\n')
            newstr = true;
    }
    
    fh.close();
    
    CONSOLE("verAvailCount founded %d versions", cnt);
    
    return cnt;
}
