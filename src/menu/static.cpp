
#include "static.h"
#include "../display.h"
#include "../gps.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../altimeter.h"
#include "../../def.h" // time/pwr


/* ------------------------------------------------------------------------------------------- *
 *  Функции стандартного текстового вывода значения
 * ------------------------------------------------------------------------------------------- */
static void valInt(char *txt, int val) {    // числовые
    sprintf_P(txt, PSTR("%d"), val);
}
static void valOn(char *txt, bool val) {    // On/Off
    strcpy_P(txt, val ? PSTR("On") : PSTR("Off"));
}
static void valYes(char *txt, bool val) {   // Yes/No
    strcpy_P(txt, val ? PSTR("Yes") : PSTR("No"));
}
static void valDsplAuto(char *txt, int8_t val) {
    switch (val) {
        case MODE_MAIN_NONE:    strcpy_P(txt, PSTR("No change")); break;
        case MODE_MAIN_LAST:    strcpy_P(txt, PSTR("Last")); break;
        case MODE_MAIN_GPS:     strcpy_P(txt, PSTR("GPS")); break;
        case MODE_MAIN_ALT:     strcpy_P(txt, PSTR("Alti")); break;
        case MODE_MAIN_ALTGPS:  strcpy_P(txt, PSTR("Alt+GPS")); break;
        case MODE_MAIN_TIME:    strcpy_P(txt, PSTR("Clock")); break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления GPS-точками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugpsupoint[] {
    {   // Выбор текущей точки
        .name = PSTR("Current"),
        .enter = NULL,
        .showval = [] (char *txt) {
            if (pnt.numValid()) {
                valInt(txt, pnt.num());
                if (!pnt.cur().used)
                    strcpy_P(txt+strlen(txt), PSTR(" (no used)"));
            }
            else
                strcpy_P(txt, PSTR("[no point]"));
        },
        .editup = [] () { pnt.numInc(); },
        .editdown = [] () { pnt.numDec(); },
    },
    {   // Сохранение текущих координат в выбранной точке
        .name = PSTR("Save position"),
        .enter = menuFlashHold, // Сохранение только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                menuFlashP(PSTR("Point not selected"));
                return;
            }
            
            TinyGPSPlus &gps = gpsGet();
            if ((gps.satellites.value() == 0) || !gps.location.isValid()) {
                // Или к моменту срабатывания длинного нажатия может не быть валидных координат (потеряны спутники)
                menuFlashP(PSTR("GPS not valid"));
                return;
            }
            
            // Сохраняем
            pnt.locSet(gps.location.lat(), gps.location.lng());
            if (!pnt.save()) {
                menuFlashP(PSTR("EEPROM fail"));
                return;
            }
            
            menuFlashP(PSTR("Point saved"));
        },
    },
    {   // Стирание координат у выбранной точки
        .name = PSTR("Clear"),
        .enter = menuFlashHold,  // Стирание только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                menuFlashP(PSTR("Point not selected"));
                return;
            }
            
            // Очищаем
            pnt.locClear();
            if (!pnt.save()) {
                menuFlashP(PSTR("EEPROM fail"));
                return;
            }
            
            menuFlashP(PSTR("Point cleared"));
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления экраном
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menudisplay[] {
    {   // Включение / выключение подсветки
        .name = PSTR("Light"),
        .enter = displayLightTgl,           // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) { valOn(txt, displayLight()); },
        //.editup = displayLightTgl,      // Сразу же применяем настройку
        //.editdown = displayLightTgl,
    },
    {   // Уровень контраста
        .name = PSTR("Contrast"),
        .enter = NULL,
        .showval = [] (char *txt) { valInt(txt, cfg.d().contrast); },
        .editup = [] () {
            if (cfg.d().contrast >= 30) return; // Значения в конфиге от 0 до 30,
                                            // а в сам драйвер пихаем уже доработанные значения, чтобы исключить
                                            // ситуацию, когда не видно совсем, что на экране
            cfg.set().contrast ++;                
            displayContrast(cfg.d().contrast);  // Сразу же применяем настройку, чтобы увидеть наглядно результат
        },
        .editdown = [] () {
            if (cfg.d().contrast <= 0) return;
            cfg.set().contrast --;
            displayContrast(cfg.d().contrast);
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню сброса нуля высотомера (на земле)
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugnd[] {
    {   // принудительная калибровка (On Ground)
        .name = PSTR("Manual set"),
        .enter = menuFlashHold,  // Сброс только по длинному нажатию
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!cfg.d().gndmanual) {
                menuFlashP(PSTR("Manual not allowed"));
                return;
            }
            
            altCalc().gndreset();
            jmpReset();
            menuFlashP(PSTR("GND corrected"));
        },
    },
    {   // разрешение принудительной калибровки: нет, "на земле", всегда
        .name = PSTR("Allow mnl set"),
        .enter = [] () {
            cfg.set().gndmanual = !cfg.d().gndmanual;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().gndmanual); },
    },
    {   // выбор автоматической калибровки: нет, автоматически на земле
        .name = PSTR("Auto correct"),
        .enter = [] () {
            cfg.set().gndauto = !cfg.d().gndauto;
        },
        .showval = [] (char *txt) { strcpy_P(txt, cfg.d().gndauto ? PSTR("On GND") : PSTR("No")); },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню автоматизации экранами с информацией
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuinfo[] {
    {   // переключать ли экран в падении автоматически в отображение только высоты
        .name = PSTR("Auto FF-screen"),
        .enter = [] () {
            cfg.set().dsplautoff = !cfg.d().dsplautoff;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().dsplautoff); },
    },
    {   // куда переключать экран под куполом: не переключать, жпс, часы, жпс+высота (по умолч)
        .name = PSTR("On CNP"),
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplcnp); },
        .editup = [] () {
            if (cfg.d().dsplcnp > MODE_MAIN_NONE)
                cfg.set().dsplcnp--;
            else
                cfg.set().dsplcnp = MODE_MAIN_MAX;
        },
        .editdown = [] () {
            if (cfg.d().dsplcnp < MODE_MAIN_MAX)
                cfg.set().dsplcnp ++;
            else
                cfg.set().dsplcnp = MODE_MAIN_NONE;
        },
    },
    {   // куда переключать экран после приземления: не переключать, жпс (по умолч), часы, жпс+высота
        .name = PSTR("After Land"),
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplland); },
        .editup = [] () {
            if (cfg.d().dsplland > MODE_MAIN_NONE)
                cfg.set().dsplland--;
            else
                cfg.set().dsplland = MODE_MAIN_MAX;
        },
        .editdown = [] () {
            if (cfg.d().dsplland < MODE_MAIN_MAX)
                cfg.set().dsplland ++;
            else
                cfg.set().dsplland = MODE_MAIN_NONE;
        },
    },
    {   // куда переключать экран при длительном бездействии на земле
        .name = PSTR("On GND"),
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplgnd); },
        .editup = [] () {
            if (cfg.d().dsplgnd > MODE_MAIN_NONE)
                cfg.set().dsplgnd--;
            else
                cfg.set().dsplgnd = MODE_MAIN_MAX;
        },
        .editdown = [] () {
            if (cfg.d().dsplgnd < MODE_MAIN_MAX)
                cfg.set().dsplgnd ++;
            else
                cfg.set().dsplgnd = MODE_MAIN_NONE;
        },
    },
    {   // куда переключать экран при включении: запоминать после выключения, все варианты экрана
        .name = PSTR("Power On"),
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplpwron); },
        .editup = [] () {
            if (cfg.d().dsplpwron > MODE_MAIN_LAST)
                cfg.set().dsplpwron--;
            else
                cfg.set().dsplpwron = MODE_MAIN_MAX;
        },
        .editdown = [] () {
            if (cfg.d().dsplpwron < MODE_MAIN_MAX)
                cfg.set().dsplpwron ++;
            else
                cfg.set().dsplpwron = MODE_MAIN_LAST;
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления часами
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menutime[] {
    {   // Выбор временной зоны, чтобы корректно синхронизироваться с службами времени
        .name = PSTR("Zone"),
        .enter = NULL,
        .showval = [] (char *txt) {
            if (cfg.d().timezone == 0) {            // cfg.timezone хранит количество минут в + или - от UTC
                strcpy_P(txt, PSTR("UTC"));     // при 0 - это время UTC
                return;
            }
            *txt = cfg.d().timezone > 0 ? '+' : '-';// Отображение знака смещения
            txt++;
        
            uint16_t m = abs(cfg.d().timezone);     // в часах и минутах отображаем пояс
            txt += sprintf_P(txt, PSTR("%d"), m / 60);
            m = m % 60;
            sprintf_P(txt, PSTR(":%02d"), m);
        },
        .editup = [] () {
            if (cfg.d().timezone >= 12*60)      // Ограничение выбора часового пояса
                return;
            cfg.set().timezone += 30;             // часовые пояса смещаем по 30 минут
            adjustTime(30 * 60);            // сразу применяем настройки
                                            // adjustTime меняет время от текущего,
                                            // поэтому надо передавать не абсолютное значение,
                                            // а смещение от текущего значения, т.е. по 30 минут
        },
        .editdown = [] () {
            if (cfg.d().timezone <= -12*60)
                return;
            cfg.set().timezone -= 30;
            adjustTime(-30 * 60);
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления питанием
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menupower[] {
    {
        .name = PSTR("Off"),
        .enter = menuFlashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = pwrOff
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления остальными системными настройками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menusystem[] {
    {
        .name = PSTR("Factory reset"),
        .enter = menuFlashHold,     // Сброс настроек только по длинному нажатию
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!cfgFactory()) {
                menuFlashP(PSTR("EEPROM fail"));
                return;
            }
            menuFlashP(PSTR("Config reseted"));
            ESP.restart();
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Главное меню конфига, тут в основном только подразделы
 * ------------------------------------------------------------------------------------------- */
#define SUBMENU(title, menu) [] () { menuEnter(new MenuStatic(PSTR(title), menu, sizeof(menu)/sizeof(menu_el_t))); }
static const menu_el_t menumain[] {
    {
        .name = PSTR("GPS points"),
        .enter = SUBMENU("GPS points", menugpsupoint),
    },
    {
        .name = PSTR("Jump count"),
        .enter = NULL,
        .showval = [] (char *txt) { valInt(txt, jmp.d().count); },
        .editup = [] () {
            jmp.set().count ++;
        },
        .editdown = [] () {
            if (jmp.d().count > 0)
                jmp.set().count --;
        },
    },
    {
        .name = PSTR("LogBook"),
        //.enter = SUBMENU(title, menu),
    },
    {
        .name = PSTR("Display"),
        .enter = SUBMENU("Display", menudisplay),
    },
    {
        .name = PSTR("Gnd Correct"),
        .enter = SUBMENU("Gnd Correct", menugnd),
    },
    {
        .name = PSTR("Auto Screen-Mode"),
        .enter = SUBMENU("Auto Screen-Mode", menuinfo),
    },
    {
        .name = PSTR("Time"),
        .enter = SUBMENU("Time", menutime),
    },
    {
        .name = PSTR("Power"),
        .enter = SUBMENU("Power", menupower),
    },
    {
        .name = PSTR("System"),
        .enter = SUBMENU("System", menusystem),
    },
    {
        .name = PSTR("Wifi sync"),
        //.enter = menuSubWifi,
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса MenuStatic
 * ------------------------------------------------------------------------------------------- */
MenuStatic::MenuStatic() :
    MenuStatic(PSTR("Configuration"), menumain, sizeof(menumain)/sizeof(menu_el_t))
{
    
}

void MenuStatic::updStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuStatic::updStr: %d\r\n", i);
    auto &m = menu[i];
    
    strncpy_P(str.name, m.name, sizeof(str.name));
    str.name[sizeof(str.name)-1] = '\0';
    
    if (m.showval == NULL)
        str.val[0] = '\0';
    else
        m.showval(str.val);
}

void MenuStatic::updHnd(int16_t i, button_hnd_t &smp, button_hnd_t &lng, button_hnd_t &editup, button_hnd_t &editdn) {
    auto &m = menu[i];
    
    smp = m.enter;
    lng = m.hold;
    editup = m.editup;
    editdn = m.editdown;
}
