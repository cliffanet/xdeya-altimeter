
#include "../eeprom.h"
#include "../display.h"

#include <EEPROM.h>

// Оперативные данные, которые надо сохранять при уходе в сон и выключении
eeprom_inf_t inf;

/* ------------------------------------------------------------------------------------------- *
 * Функции чтения и сохранения конфига в eeprom
 * ------------------------------------------------------------------------------------------- */
void infLoad() {
    EEPROMClass eep(EEPROM_INF_NAME, 0);
    eep.begin(EEPROM_INF_SIZE);
    auto *e = eep.getDataPtr();
    eeprom_inf_t *_inf = (eeprom_inf_t *)e;

    if ((_inf->mgc1 == EEPROM_MGC1) && (e[EEPROM_INF_SIZE-1] == EEPROM_MGC2)) {
        inf = *_inf;
    }
    else {
        eeprom_inf_t _inf;
        inf = _inf;
    }
    eep.end();
}

void infSave() {
    EEPROMClass eep(EEPROM_INF_NAME, 0);
    eep.begin(EEPROM_INF_SIZE);
    auto *e = eep.getDataPtr();
    bzero(e, EEPROM_INF_SIZE);
    memcpy(e, &inf, sizeof(inf));
    e[EEPROM_INF_SIZE-1] = EEPROM_MGC2;
    eep.commit();
    eep.end();
    Serial.println("info saved to eeprom");
}
