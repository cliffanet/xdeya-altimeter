

#include "sys.h"

#if HWVER >= 5
#include "../log.h"
#include "../core/file.h"
#include "../view/text.h"

#include <Update.h>         // Обновление прошивки
#include <SD.h>

#define ERR(s)              err(PSTR(TXT_FWSD_ERR_ ## s))

WrkFwUpdCard::WrkFwUpdCard(const char *fname) {
    char name[64];
    snprintf_P(name, sizeof(name), PSTR("/%s"), fname);
    fh = SD.open(name);

    strncpy(m_str, fname, sizeof(m_str));
    m_str[sizeof(m_str)-1] = '\0';
}

Wrk::state_t WrkFwUpdCard::run() {
WPROC
    if (!fh)
        return ERR(FILEOPEN);
    CONSOLE("fname: %s", fh.name());
    
    // size
    m_cmpl.sz = fh.size();
    uint32_t freesz = ESP.getFreeSketchSpace();
    uint32_t cursz = ESP.getSketchSize();
    CONSOLE("current fw size: %lu, avail size for new fw: %lu, new fw size: %lu", 
            cursz, freesz, fh.size());
    
    if (fh.size() > freesz)
        return ERR(FWSIZEBIG);

WPRC_RUN
    char md5[64] = { '/' };
    bool usemd5 = false;
    
    // md5
    strcpy(md5+1, fh.name());
    int len = strlen(md5);
    md5[len-3] = 'm';
    md5[len-2] = 'd';
    md5[len-1] = '5';
    if (SD.exists(md5)) {
        File fh1 = SD.open(md5);
        if (!fh1)
            return ERR(FILEOPEN);
        if (fh1.read(reinterpret_cast<uint8_t *>(md5), 32) != 32)
            return ERR(FILEREAD);
        fh1.close();
        
        md5[32] = '\0';
        if (strlen(md5) != 32)
            return ERR(MD5);
        usemd5 = true;
        CONSOLE("md5: %s", md5);
    }
    else
        CONSOLE("no md5");

    // start burn
    if (!Update.begin(fh.size(), U_FLASH)) {
        CONSOLE("Upd begin fail: errno=%d", Update.getError());
        return ERR(FWINIT);
    }
    if (usemd5 && !Update.setMD5(md5)) {
        CONSOLE("Upd md5 fail: errno=%d", Update.getError());
        return ERR(MD5);
    }

WPRC_RUN
    uint8_t buf[512];
    int sz = fh.read(buf, sizeof(buf));

    if (sz < 1)
        return ERR(FILEREAD);
    if (Update.write(buf, sz) != sz)
        return ERR(FWWRITE);
    
    m_cmpl.val += sz;
    
    if (fh.available() > 0)
        return RUN;
    
WPRC_RUN
    if (!Update.end())
        return ERR(FWFIN);
    
    ESP.restart();
    m_isok=true;

WPRC(END)
};

void WrkFwUpdCard::end() {
    fh.close();
    fileExtStop();
}

#endif // HWVER >= 5
