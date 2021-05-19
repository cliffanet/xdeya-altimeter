
#include "log.h"
#include "wifi.h"
#include "../log.h"

#define FNAMEINIT   \
    char fname[20]; \
    fname[0] = '/'; \
    strcpy_P(fname+1, PSTR(WIFIPASS_FILE));

/* ------------------------------------------------------------------------------------------- *
 *  Удаление файла
 * ------------------------------------------------------------------------------------------- */
bool wifiPassClear() {
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return true;
    return DISKFS.remove(fname);
}

/* ------------------------------------------------------------------------------------------- *
 *  Добавление сети с паролем
 * ------------------------------------------------------------------------------------------- */
bool wifiPassAdd(const char *ssid, const char *pass) {
    FNAMEINIT
    
    File fh = DISKFS.open(fname, FILE_APPEND);
    if (!fh)
        return false;
    
    CONSOLE("save wifi: %s; pass: |%s|", ssid, pass);
    
    fh.print(F("ssid "));
    fh.println(ssid);
    fh.print(F("pass "));
    fh.println(pass);
    fh.close();
    
    return true;
}


/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма списка сетей
 * ------------------------------------------------------------------------------------------- */
uint32_t wifiPassChkSum() {
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
 *  Поиск пароля по имени wifi-сети
 * ------------------------------------------------------------------------------------------- */
bool wifiPassFind(const char *ssid, char *pass) {
    FNAMEINIT
    
    if (!DISKFS.exists(fname))
        return false;
    
    File fh = DISKFS.open(fname);
    if (!fh)
        return false;
    
#define ISLINEEND(c) ((c == '\r') || (c == '\n'))
#define ISSPACE(c)   ((c == ' ') || (c == '\t'))
    
    char s[512], *ss;
    int sz;
    bool founded = false;
    
    CONSOLE("wifiPassFind search: |%s|", ssid);
    
    while (fh.available() > 0) {
        ss = s;
        sz = sizeof(s);
        while (fh.available() > 0) {
            *ss = fh.read(); // читаем до окончания строки
            if (ISLINEEND(*ss)) {
                if (ss == s) // строка ещё не началась∂
                    continue;
                else
                    break;
            }
            if ((ss == s) && ISSPACE(*ss)) // пробелы в начале строки
                continue;
            ss ++;
            sz --;
            if (sz <= 1) {
                // закончился буфер
                while (fh.available() > 0) { // дочитываем файл до конца строки
                    *ss = fh.read();
                    if (ISLINEEND(*ss))
                        break;
                }
                break;
            }
            if (fh.available() <= 0) // закончился файл
                break;
        }
        *ss = '\0';
        
        if (strncmp_P(s, PSTR("ssid "), 5) == 0) {
            ss = s + 5;
            if (founded) // сеть уже была найдена ранее, но пароль так и не получен, прерываем поиск совсем
                break;
            while (*ss && ISSPACE(*ss)) ss++; // пропускаем пробелы
            founded = strcmp(ss, ssid) == 0;
        }
        else
        if (!founded) {
            continue;
        }
        else
        if (strncmp_P(s, PSTR("pass "), 5) == 0) {
            ss = s + 5;
            CONSOLE("wifiPassFind found pass: |%s|", ss);
            if (pass != NULL) {
                while (*ss && ISSPACE(*ss)) ss++; // пропускаем пробелы
                strncpy(pass, ss, 32);
                pass[32] = '\0';
                CONSOLE("wifiPassFind return: |%s|", pass);
            }
            fh.close();
            return true;
        }
    }
    
    fh.close();
    
    CONSOLE("wifiPassFind not found");
    
    return false;
}
