
#include "main.h"
#include "../log.h"
#include "point.h"
#include "jump.h"
#include "webjoin.h"
#include "../gps/proc.h"
#include "../file/log.h"
#include "../view/base.h"

#include <FS.h>
#include <SPIFFS.h>

template class Config<cfg_main_t>;
Config<cfg_main_t> cfg(PSTR(CFG_MAIN_NAME), CFG_MAIN_ID, CFG_MAIN_VER);

template class Config<cfg_point_t>; // Почему-то не линкуется, если эту строчку переместить в point.cpp

template class Config<cfg_jump_t>;

template class Config<cfg_webjoin_t>;

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса Config
 * ------------------------------------------------------------------------------------------- */

template <typename T>
Config<T>::Config(const char *_fname, uint8_t _cfgid, uint8_t _ver) {
    fname[0] = '/';
    strncpy_P(fname+1, _fname, sizeof(fname)-1);
    fname[sizeof(fname)-1] = '\0';
    cfgid = _cfgid;
    ver = _ver;
    reset();
}

template <typename T>
bool Config<T>::load() {
    if (!SPIFFS.exists(fname)) {
        reset();
        _modifed = false;
        CONSOLE("config %s not exists: default", fname);
        return true;
    }
    
    File fh = SPIFFS.open(fname, FILE_READ);
    if (!fh)
        return false;
    
    uint8_t h[2];
    size_t sz = fh.read(h, 2);
    if (sz != 2) {
        CONSOLE("config %s header fail: %d of %d", fname, sz, 2);
        fh.close();
        return false;
    }
    
    uint8_t id1 = h[1] >> 5;
    uint8_t ver1 = h[1] & 0x1f;
    if ((h[0] != CFG_MGC) || (id1 != cfgid) || (ver1 == 0)) {
        CONSOLE("config %s is not valid: mgc=0x%02x, h2=0x%02x, cfgid=%d, ver=%d", fname, h[0], h[1], id1, ver1);
        reset();
        fh.close();
        return false;
    }
    
    if (ver1 > ver) {
        CONSOLE("config %s version later: ver=%d", fname, ver1);
        reset();
        fh.close();
        return false;
    }
    
    if (ver1 < ver) {
        CONSOLE("config %s version earler: ver=%d", fname, ver1);
        uint8_t buf[sizeof(T)*2];
        if (!fread(fh, buf, sizeof(buf))) {
            fh.close();
            return false;
        }
        upgrade(ver1, buf);
    }
    else {
        T d;
        if (!fread(fh, d, false)) {
            fh.close();
            return false;
        }
        data = d;
    }

    fh.close();
    
    CONSOLE("config %s read ok", fname);
    
    _modifed = false;
    return true;
}

template <typename T>
bool Config<T>::save(bool force) {
    if (!force && !_modifed) {
        CONSOLE("config %s not changed", fname);
        return true;
    }

    viewIntDis();
    if (SPIFFS.exists(fname) && !SPIFFS.remove(fname)) {
        viewIntEn();
        return false;
    }
    
    File fh = SPIFFS.open(fname, FILE_WRITE);
    if (!fh) {
        viewIntEn();
        return false;
    }

    uint8_t h[2] = { CFG_MGC, (cfgid << 5) | (ver & 0x1f) };
    
    size_t sz = fh.write(h, 2);
    if (sz != 2) {
        CONSOLE("config %s header fail: %d of %d", fname, sz, 2);
        fh.close();
        viewIntEn();
        return false;
    }
    
    if (!fwrite(fh, data)) {
        fh.close();
        viewIntEn();
        return false;
    }
    
    fh.close();
    viewIntEn();
    
    CONSOLE("config %s saved OK", fname);
    
    _modifed = false;
    return true;
}

template <typename T>
void Config<T>::reset() {
    T d1;
    data = d1;
    _modifed = true;
}

template <typename T>
void Config<T>::upgrade(uint8_t v, const uint8_t *d) {
    memcpy(&data, d, sizeof(data));
}



template <typename T>
uint32_t Config<T>::chksum() const {
    uint32_t sz = sizeof(data);
    const uint8_t *d = reinterpret_cast<const uint8_t *>(&data);
    
    uint16_t cka = 0, ckb;
    for (int i=0; i<sz; i++) {
        cka += d[i];
        ckb += cka;
    }
    
    return (cka << 16) | ckb;
}

bool cfgLoad(bool apply) {
    bool ok = true;
    if (!cfg.load())
        ok = false;
    if (!pnt.load())
        ok = false;
    if (!jmp.load())
        ok = false;
    
    if (apply) {
        displayContrast(cfg.d().contrast);
        displayFlipp180(cfg.d().flipp180);
        btnFlipp180(cfg.d().flipp180);
        if (cfg.d().gpsonpwron)
            gpsOn(GPS_PWRBY_PWRON);
    }
    
    return ok;
}
bool cfgSave(bool force) {
    return
        cfg.save(force) &&
        pnt.save(force) &&
        jmp.save(force);
}

bool cfgFactory() {
    /*
    cfg.reset();
    pnt.reset();
    jmp.reset();
    
    return cfgSave(true);
    */
    SPIFFS.end();
    CONSOLE("SPIFFS Unmount ok");
    viewIntDis();
    if (!SPIFFS.format()) {
        CONSOLE("SPIFFS Format Failed");
        viewIntEn();
        return false;
    }
    CONSOLE("SPIFFS Format ok");

    if(!SPIFFS.begin()) {
        CONSOLE("SPIFFS Mount Failed");
        viewIntEn();
        return false;
    }
    CONSOLE("SPIFFS begin ok");
    viewIntEn();
    
    return cfgLoad(true);
}
