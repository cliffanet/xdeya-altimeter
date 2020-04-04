
#include "main.h"
#include "point.h"
#include "jump.h"
#include "webjoin.h"
#include "../display.h"

#include <FS.h>
#include <SPIFFS.h>

template class Config<cfg_main_t>;
Config<cfg_main_t> cfg(PSTR(CFG_MAIN_NAME), CFG_MAIN_VER);

template class Config<cfg_point_t>; // Почему-то не линкуется, если эту строчку переместить в point.cpp

template class Config<cfg_jump_t>;

template class Config<cfg_webjoin_t>;

template class Config<cfg_info_t>;
Config<cfg_info_t> inf(PSTR(CFG_INFO_NAME), CFG_INFO_VER);

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
    Serial.print("config ");
    Serial.print(fname);
    Serial.println(" not exists: default");
        return true;
    }
    
    File fh = SPIFFS.open(fname, FILE_READ);
    if (!fh)
        return false;
    
    size_t sz = fh.read(reinterpret_cast<uint8_t *>(&data), sizeof(data));
    fh.close();
    
    bool ok =
        (sz == sizeof(data)) &&
        (data.mgc1 == CFG_MGC1) &&
        (data.ver == ver) &&
        (data.mgc2 == CFG_MGC2);
    
    Serial.print("config ");
    Serial.print(fname);
    Serial.print(" read: ");
    Serial.println(ok);
        
    _modifed = !ok;
    return ok;
}

template <typename T>
bool Config<T>::save(bool force) {
    if (!force && !_modifed) {
    Serial.print("config ");
    Serial.print(fname);
    Serial.println(" not changed");
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
    
    Serial.print("config ");
    Serial.print(fname);
    Serial.println(" saved OK");
    
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


bool cfgLoad(bool apply) {
    if (!cfg.load() ||
        !pnt.load() ||
        !jmp.load() ||
        !inf.load())
        return false;
    if (apply) {
        displayContrast(cfg.d().contrast);
    }
    return true;
}
bool cfgSave(bool force) {
    return
        cfg.save(force) &&
        pnt.save(force) &&
        jmp.save(force) &&
        inf.save(force);
}

bool cfgFactory() {
    cfg.reset();
    pnt.reset();
    jmp.reset();
    inf.reset();
    
    return cfgSave(true);
}
