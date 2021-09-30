
#include "main.h"
#include "../log.h"
#include "point.h"
#include "jump.h"
#include "webjoin.h"

#include <FS.h>
#include <SPIFFS.h>

template class Config<cfg_main_t>;
Config<cfg_main_t> cfg(PSTR(CFG_MAIN_NAME), CFG_MAIN_VER);

template class Config<cfg_point_t>; // Почему-то не линкуется, если эту строчку переместить в point.cpp

template class Config<cfg_jump_t>;

template class Config<cfg_webjoin_t>;

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса Config
 * ------------------------------------------------------------------------------------------- */

template <typename T>
Config<T>::Config(const char *_fname, uint8_t _ver) {
    fname[0] = '/';
    strncpy_P(fname+1, _fname, sizeof(fname)-1);
    fname[sizeof(fname)-1] = '\0';
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
    
    T d;
    
    size_t sz = fh.read(reinterpret_cast<uint8_t *>(&d), sizeof(T));
    fh.close();
    
    bool ok =
        (sz == sizeof(T)) &&
        (d.mgc1 == CFG_MGC1) &&
        (d.ver == ver) &&
        (d.mgc2 == CFG_MGC2);
    
    CONSOLE("config %s read: %d", fname, ok);
    
    if (ok)
        data = d;
    
    _modifed = !ok;
    return ok;
}

template <typename T>
bool Config<T>::save(bool force) {
    if (!force && !_modifed) {
        CONSOLE("config %s not changed", fname);
        return true;
    }
    
    if (SPIFFS.exists(fname) && !SPIFFS.remove(fname))
        return false;
    
    File fh = SPIFFS.open(fname, FILE_WRITE);
    if (!fh)
        return false;
    
    size_t sz = fh.write(reinterpret_cast<uint8_t *>(&data), sizeof(data));
    fh.close();
    
    if (sz != sizeof(data))
        return false;
    
    CONSOLE("config %s saved OK", fname);
    
    _modifed = false;
    return true;
}

template <typename T>
void Config<T>::reset() {
    T d1;
    data = d1;
    data.mgc1 = CFG_MGC1;
    data.ver = ver;
    data.mgc2 = CFG_MGC2;
    _modifed = true;
}

template <typename T>
uint32_t Config<T>::chksum() const {
    uint32_t sz = sizeof(data);
    const uint8_t *d = reinterpret_cast<const uint8_t *>(&data);
    
    uint16_t sum = 0;
    for (int i=0; i<sz; i++)
        sum += d[i];
    
    return (sum << 16) | (sz & 0xffff);
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
    if (!SPIFFS.format()) {
        CONSOLE("SPIFFS Format Failed");
        return false;
    }
    CONSOLE("SPIFFS Format ok");

    if(!SPIFFS.begin()) {
        CONSOLE("SPIFFS Mount Failed");
        return false;
    }
    CONSOLE("SPIFFS begin ok");
    
    return cfgLoad(true);
}
