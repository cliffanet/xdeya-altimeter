
#include "def.h"

#include <EEPROM.h>

// конфиг, сохраняемый в eeprom
eeprom_cfg_t cfg;

/* ------------------------------------------------------------------------------------------- *
 * Функции чтения и сохранения конфига в eeprom
 * ------------------------------------------------------------------------------------------- */
void cfgLoad() {
    EEPROM.begin(sizeof(eeprom_cfg_t));
    eeprom_cfg_t *_cfg = (eeprom_cfg_t *)EEPROM.getDataPtr();

    if ((_cfg->mgc1 == EEPROM_MGC1) && (_cfg->ver == EEPROM_VER) && (_cfg->mgc2 == EEPROM_MGC2)) {
        cfg = *_cfg;
    }
    else {
        eeprom_cfg_t _cfg;
        cfg = _cfg;
    }
    EEPROM.end();
}

void cfgSave() {
    EEPROM.begin(sizeof(eeprom_cfg_t));
    *((eeprom_cfg_t *)EEPROM.getDataPtr()) = cfg;
    EEPROM.commit();
    EEPROM.end();
    Serial.print("config saved to eeprom");
}

void cfgFactory() {
    eeprom_cfg_t _cfg;
    cfg = _cfg;
    cfgSave();
    //cfgApply();
    ESP.restart();
}
void cfgApply() {
    displayContrast(cfg.contrast);
}
