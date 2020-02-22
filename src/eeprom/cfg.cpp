
#include "../eeprom.h"
#include "../display.h"

#include <EEPROM.h>

// конфиг, сохраняемый в eeprom
eeprom_cfg_t cfg;

static EEPROMClass EEPcfg(EEPROM_CFG_NAME, 0);

/* ------------------------------------------------------------------------------------------- *
 * Функции чтения и сохранения конфига в eeprom
 * ------------------------------------------------------------------------------------------- */
void cfgLoad() {
    EEPcfg.begin(EEPROM_CFG_SIZE);
    eeprom_cfg_t *_cfg = (eeprom_cfg_t *)EEPcfg.getDataPtr();

    if ((_cfg->mgc1 == EEPROM_MGC1) && (_cfg->ver == EEPROM_VER)) {
        cfg = *_cfg;
    }
    else {
        eeprom_cfg_t _cfg;
        cfg = _cfg;
    }
    EEPcfg.end();
}

void cfgSave() {
    EEPcfg.begin(EEPROM_CFG_SIZE);
    *((eeprom_cfg_t *)EEPcfg.getDataPtr()) = cfg;
    EEPcfg.commit();
    EEPcfg.end();
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
