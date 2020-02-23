
#include "../eeprom.h"
#include "../display.h"

#include <EEPROM.h>

// конфиг, сохраняемый в eeprom
eeprom_cfg_t cfg;

/* ------------------------------------------------------------------------------------------- *
 * Функции чтения и сохранения конфига в eeprom
 * ------------------------------------------------------------------------------------------- */
void cfgLoad() {
    EEPROMClass eep(EEPROM_CFG_NAME, 0);
    eep.begin(EEPROM_CFG_SIZE);
    auto *e = eep.getDataPtr();
    eeprom_cfg_t *_cfg = (eeprom_cfg_t *)e;

    if ((_cfg->mgc1 == EEPROM_MGC1) && (_cfg->ver == EEPROM_VER) && (e[EEPROM_CFG_SIZE-1] == EEPROM_MGC2)) {
        cfg = *_cfg;
    }
    else {
        eeprom_cfg_t _cfg;
        cfg = _cfg;
    }
    eep.end();
}

void cfgSave() {
    EEPROMClass eep(EEPROM_CFG_NAME, 0);
    eep.begin(EEPROM_CFG_SIZE);
    auto *e = eep.getDataPtr();
    bzero(e, EEPROM_CFG_SIZE);
    memcpy(e, &cfg, sizeof(cfg));
    e[EEPROM_CFG_SIZE-1] = EEPROM_MGC2;
    eep.commit();
    eep.end();
    Serial.println("config saved to eeprom");
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
