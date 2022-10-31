
#include "main.h"
#include "../log.h"
#include "point.h"
#include "jump.h"
#include "webjoin.h"
#include "../navi/proc.h"
#include "../core/filebin.h"
#include "../view/base.h"
#include "../navi/compass.h"
#include "../net/bt.h"

#include <SPIFFS.h> // cfg reset default = format

template class Config<cfg_main_t>;
Config<cfg_main_t> cfg(PSTR(CFG_MAIN_NAME), CFG_MAIN_ID, CFG_MAIN_VER);

template class Config<cfg_point_t>; // Почему-то не линкуется, если эту строчку переместить в point.cpp

template class Config<cfg_jump_t>;

template class Config<cfg_webjoin_t>;

template class Config<cfg_magcal_t>;

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса Config
 * ------------------------------------------------------------------------------------------- */

template <typename T>
Config<T>::Config(const char *_fname_P, uint8_t _cfgid, uint8_t _ver) :
    fname_P(_fname_P),
    cfgid(_cfgid),
    ver(_ver)
{
    reset();
}

template <typename T>
bool Config<T>::load() {
    if (!fileExists(fname_P)) {
        reset();
        _modifed = false;
        CONSOLE("config [%d] not exists: default", cfgid);
        return true;
    }
    
    FileBin f(fname_P);
    if (!f)
        return false;
    
    uint8_t h[2];
    size_t sz = f.read(h, 2);
    if (sz != 2) {
        CONSOLE("config [%d] header fail: %d of %d", cfgid, sz, 2);
        f.close();
        return false;
    }
    
    uint8_t id1 = h[1] >> 5;
    uint8_t ver1 = h[1] & 0x1f;
    if ((h[0] != CFG_MGC) || (id1 != cfgid) || (ver1 == 0)) {
        CONSOLE("config [%d] is not valid: mgc=0x%02x, h2=0x%02x, cfgid=%d, ver=%d", cfgid, h[0], h[1], id1, ver1);
        reset();
        f.close();
        return false;
    }
    
    if (ver1 > ver) {
        CONSOLE("config [%d] version later: ver=%d", cfgid, ver1);
        reset();
        f.close();
        return false;
    }
    
    if (ver1 < ver) {
        CONSOLE("config [%d] version earler: ver=%d", cfgid, ver1);
        uint8_t buf[sizeof(T)*2];
        if (!f.get(buf, sizeof(buf))) {
            f.close();
            return false;
        }
        upgrade(ver1, buf);
    }
    else {
        T d;
        if (!f.get(d, false)) {
            f.close();
            return false;
        }
        data = d;
    }

    f.close();
    
    CONSOLE("config [%d] read ok", cfgid);
    
    _modifed = false;
    _empty = false;
    return true;
}

template <typename T>
bool Config<T>::save(bool force) {
    if (!force && !_modifed) {
        CONSOLE("config [%d] not changed", cfgid);
        return true;
    }

    if (fileExists(fname_P) && !fileRemove(fname_P)) {
        CONSOLE("config [%d] remove fail", cfgid);
        return false;
    }
    
    FileBin f(fname_P, FileMy::MODE_WRITE);
    if (!f)
        return false;

    uint8_t h[2] = { CFG_MGC, static_cast<uint8_t>((cfgid << 5) | (ver & 0x1f)) };
    
    size_t sz = f.write(h, 2);
    if (sz != 2) {
        CONSOLE("config [%d] header fail: %d of %d", cfgid, sz, 2);
        f.close();
        return false;
    }
    
    if (!f.add(data)) {
        f.close();
        return false;
    }
    
    f.close();
    
    CONSOLE("config [%d] saved OK", cfgid);
    
    _modifed = false;
    return true;
}

template <typename T>
void Config<T>::reset() {
    T d1;
    data = d1;
    _modifed = true;
    _empty = true;
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
        if (cfg.d().navonpwron)
            gpsOn(NAV_PWRBY_PWRON);
        
#ifdef USE_BLUETOOTH
        if ((cfg.d().net & 0x1) > 0)
            bluetoothStart();
#endif // #ifdef USE_BLUETOOTH
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
