
#include "menu.h"
#include "../log.h"
#include "../power.h"
#include "../clock.h"
#include "../view/base.h"
#include "../gps/proc.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../jump/proc.h"
#include "../file/track.h"
#include "../../def.h" // time/pwr


typedef void (*menu_hnd_t)();
typedef void (*menu_edit_t)(int val);
typedef void (*menu_val_t)(char *txt);

// Элемент меню
typedef struct {
    char        *name;      // Текстовое название пункта
    ViewMenu    *submenu;   // вложенное подменю
    menu_hnd_t  enter;      // Обраотка нажатия на среднюю кнопку
    menu_val_t  showval;    // как отображать значение, если требуется
    menu_edit_t edit;       // в режиме редактирования меняет значения на величину val (функция должна описывать, что и как менять)
    menu_hnd_t  hold;       // Обработка действия при длинном нажатии на среднюю кнопку
} menu_el_t;

class ViewMenuStatic : public ViewMenu {
    public:
        ViewMenuStatic(const menu_el_t *m, int16_t sz, bool autoupdate = false) : ViewMenu(sz), menu(m), aupd(autoupdate) {};
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            //CONSOLE("ViewMenuStatic::getStr: %d", i);
            auto &m = menu[i];
    
            strncpy_P(str.name, m.name, sizeof(str.name));
            str.name[sizeof(str.name)-1] = '\0';
    
            if (m.showval == NULL)
                str.val[0] = '\0';
            else
                m.showval(str.val);
        }
        bool valFlash() { return isedit; }
        
        void btnSmpl(btn_code_t btn) {
            menuFlashClear();
            
            if (isedit) {
                // режим редактирования
                auto ehnd = isExit(isel) ?
                    NULL : menu[sel()].edit;
                if (ehnd == NULL)
                    btn = BTN_SEL;
                switch (btn) {
                    case BTN_UP:
                        ehnd(+1);
                        break;
                        
                    case BTN_SEL:
                        isedit = false;
                        break;
                    
                    case BTN_DOWN:
                        ehnd(-1);
                        break;
                }
                updStrSel();
                return;
            }
            
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel)) {
                // Сохраняем настройки, если изменены
                if (!cfgSave()) {
                    menuFlashP(PSTR("Config save error"));
                    return;
                }
                ViewMenu::btnSmpl(btn);
                return;
            }
            
            auto &m = menu[sel()];
            
            if (m.edit != NULL) {
                isedit = true;
                updStrSel();
                return;
            }
            
            if (m.submenu != NULL) {
                viewSet(*(m.submenu));
                m.submenu->open(this, m.name);
                return;
            }
            
            if (m.enter != NULL)
                m.enter();
            
            updStr(); // полностью обновляем экран после клика
        }
        
        bool useLong(btn_code_t btn) {
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel))
                return false;
            return menu[sel()].hold != NULL;
        }

        void btnLong(btn_code_t btn) {
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel))
                return;
            auto &m = menu[sel()];
            if (m.hold != NULL)
                m.hold();
            updStr(); // полностью обновляем экран после клика
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT) {
                cfgSave(); //  сохраняем конфиг, если изменён, но в случае ошибки игнорим
                setViewMain();
            }
    
            if (isedit) {
                // Обработка удержания кнопок вверх/вниз в режиме редактирования
                uint8_t btn = 0;
                auto t = btnPressed(btn);
                int sign = btn == BTN_UP ? 1 : btn == BTN_DOWN ? -1 : 0;
                auto ehnd = isExit(isel) ?
                    NULL : menu[sel()].edit;
                if ((sign != 0) && (ehnd != NULL)) {
                    if (t > 3000) {
                        ehnd(sign * 100);
                        updStrSel();
                    }
                    else
                    if (t > 1000) {
                        ehnd(sign * 10);
                        updStrSel();
                    }
                }
            }
            
            if (aupd)
                updStr();
        }
    
    private:
        const menu_el_t *menu;
        bool isedit = false, aupd = false;
};

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
static void valOk(char *txt, bool val) {    // On/Off
    strcpy_P(txt, val ? PSTR("OK") : PSTR("fail"));
}
static void valDsplAuto(char *txt, int8_t val) {
    switch (val) {
        case MODE_MAIN_NONE:    strcpy_P(txt, PSTR("No change")); break;
        case MODE_MAIN_LAST:    strcpy_P(txt, PSTR("Last")); break;
        case MODE_MAIN_GPS:     strcpy_P(txt, PSTR("GPS")); break;
        case MODE_MAIN_ALT:     strcpy_P(txt, PSTR("Alti")); break;
        case MODE_MAIN_GPSALT:  strcpy_P(txt, PSTR("GPS+Alt")); break;
        case MODE_MAIN_TIME:    strcpy_P(txt, PSTR("Clock")); break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления GPS-точками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugpsupoint[] {
    {   // Выбор текущей точки
        .name = PSTR("Current"),
        .submenu = NULL,
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
        .edit = [] (int val) {
            if (val == 1)
                pnt.numDec();
            else
            if (val == -1)
                pnt.numInc();
        },
    },
    {   // Сохранение текущих координат в выбранной точке
        .name = PSTR("Save position"),
        .submenu = NULL,
        .enter = menuFlashHold, // Сохранение только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                menuFlashP(PSTR("Point not selected"));
                return;
            }
            
            auto &gps = gpsInf();
            if (!GPS_VALID_LOCATION(gps)) {
                // Или к моменту срабатывания длинного нажатия может не быть валидных координат (потеряны спутники)
                menuFlashP(PSTR("GPS not valid"));
                return;
            }
            
            // Сохраняем
            pnt.locSet(GPS_LATLON(gps.lat), GPS_LATLON(gps.lon));
            if (!pnt.save()) {
                menuFlashP(PSTR("EEPROM fail"));
                return;
            }
            
            menuFlashP(PSTR("Point saved"));
        },
    },
    {   // Стирание координат у выбранной точки
        .name = PSTR("Clear"),
        .submenu = NULL,
        .enter = menuFlashHold,  // Стирание только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .edit = NULL,
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
static ViewMenuStatic vMenuGpsPoint(menugpsupoint, sizeof(menugpsupoint)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню работы с трэками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menutrack[] {
    {   // Текущий режим записи трэка
        .name = PSTR("Recording"),
        .submenu = NULL,
        .enter = menuFlashHold,           // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) {
            switch (trkState()) {
                case TRKRUN_NONE:  strcpy_P(txt, PSTR("no")); break;
                case TRKRUN_FORCE: strcpy_P(txt, PSTR("Force")); break;
                case TRKRUN_AUTO:  strcpy_P(txt, PSTR("Auto")); break;
                default: txt[0] = '\0';
            }
        },
        .edit = NULL,
        .hold = [] () {
            if (trkRunning())
                trkStop();
            else
                trkStart(true);
        },
    },
    {   // Количество записей
        .name = PSTR("Count"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valInt(txt, trkFileCount()); },
    },
    {   // Сколько времени доступно
        .name = PSTR("Avail"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            // сколько записей ещё влезет
            size_t avail = trkCountAvail() / 5; // количество секунд
            if (avail >= 60)
                sprintf_P(txt, PSTR("%d:%02d"), avail/60, avail%60);
            else
                sprintf_P(txt, PSTR("%d s"), avail);
        },
    },
};
static ViewMenuStatic vMenuTrack(menutrack, sizeof(menutrack)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления экраном
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menudisplay[] {
    {   // Включение / выключение подсветки
        .name = PSTR("Light"),
        .submenu = NULL,
        .enter = displayLightTgl,           // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) { valOn(txt, displayLight()); },
    },
    {   // Уровень контраста
        .name = PSTR("Contrast"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valInt(txt, cfg.d().contrast); },
        .edit = [] (int val) {
            int c = cfg.d().contrast;
            c+=val;
            if (c > 30) c = 30; // Значения в конфиге от 0 до 30
            if (c < 0) c = 0;
            if (cfg.d().contrast == c) return;
            displayContrast(cfg.set().contrast = c);  // Сразу же применяем настройку, чтобы увидеть наглядно результат
        },
    },
};
static ViewMenuStatic vMenuDisplay(menudisplay, sizeof(menudisplay)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню сброса нуля высотомера (на земле)
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugnd[] {
    {   // принудительная калибровка (On Ground)
        .name = PSTR("Manual set"),
        .submenu = NULL,
        .enter = menuFlashHold,  // Сброс только по длинному нажатию
        .showval = NULL,
        .edit = NULL,
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
        .submenu = NULL,
        .enter = [] () {
            cfg.set().gndmanual = !cfg.d().gndmanual;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().gndmanual); },
    },
    {   // выбор автоматической калибровки: нет, автоматически на земле
        .name = PSTR("Auto correct"),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().gndauto = !cfg.d().gndauto;
        },
        .showval = [] (char *txt) { strcpy_P(txt, cfg.d().gndauto ? PSTR("On GND") : PSTR("No")); },
    },
};
static ViewMenuStatic vMenuGnd(menugnd, sizeof(menugnd)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню автоматизации экранами с информацией
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuinfo[] {
    {   // переключать ли экран в падении автоматически в отображение только высоты
        .name = PSTR("Auto FF-screen"),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().dsplautoff = !cfg.d().dsplautoff;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().dsplautoff); },
    },
    {   // куда переключать экран под куполом: не переключать, жпс, часы, жпс+высота (по умолч)
        .name = PSTR("On CNP"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplcnp); },
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().dsplcnp > MODE_MAIN_NONE)
                    cfg.set().dsplcnp--;
                else
                    cfg.set().dsplcnp = MODE_MAIN_MAX;
            }
            else
            if (val == 1) {
                if (cfg.d().dsplcnp < MODE_MAIN_MAX)
                    cfg.set().dsplcnp ++;
                else
                    cfg.set().dsplcnp = MODE_MAIN_NONE;
            }
        },
    },
    {   // куда переключать экран после приземления: не переключать, жпс (по умолч), часы, жпс+высота
        .name = PSTR("After Land"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplland); },
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().dsplland > MODE_MAIN_NONE)
                    cfg.set().dsplland--;
                else
                    cfg.set().dsplland = MODE_MAIN_MAX;
            }
            else
            if (val == 1) {
                if (cfg.d().dsplland < MODE_MAIN_MAX)
                    cfg.set().dsplland ++;
                else
                    cfg.set().dsplland = MODE_MAIN_NONE;
            }
        },
    },
    {   // куда переключать экран при длительном бездействии на земле
        .name = PSTR("On GND"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplgnd); },
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().dsplgnd > MODE_MAIN_NONE)
                    cfg.set().dsplgnd--;
                else
                    cfg.set().dsplgnd = MODE_MAIN_MAX;
            }
            else
            if (val == 1) {
                if (cfg.d().dsplgnd < MODE_MAIN_MAX)
                    cfg.set().dsplgnd ++;
                else
                    cfg.set().dsplgnd = MODE_MAIN_NONE;
            }
        },
    },
    {   // куда переключать экран при включении: запоминать после выключения, все варианты экрана
        .name = PSTR("Power On"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valDsplAuto(txt, cfg.d().dsplpwron); },
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().dsplpwron > MODE_MAIN_LAST)
                    cfg.set().dsplpwron--;
                else
                    cfg.set().dsplpwron = MODE_MAIN_MAX;
            }
            else
            if (val == 1) {
                if (cfg.d().dsplpwron < MODE_MAIN_MAX)
                    cfg.set().dsplpwron ++;
                else
                    cfg.set().dsplpwron = MODE_MAIN_LAST;
            }
        },
    },
};
static ViewMenuStatic vMenuInfo(menuinfo, sizeof(menuinfo)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления часами
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menutime[] {
    {   // Выбор временной зоны, чтобы корректно синхронизироваться с службами времени
        .name = PSTR("Zone"),
        .submenu = NULL,
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
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().timezone >= 12*60)      // Ограничение выбора часового пояса
                    return;
                cfg.set().timezone += 30;             // часовые пояса смещаем по 30 минут
                clockForceAdjust();            // сразу применяем настройки
            }
            else
            if (val == 1) {
                if (cfg.d().timezone <= -12*60)
                    return;
                cfg.set().timezone -= 30;
                clockForceAdjust();
            }
        },
    },
};
static ViewMenuStatic vMenuTime(menutime, sizeof(menutime)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления питанием
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menupower[] {
    {
        .name = PSTR("Off"),
        .submenu = NULL,
        .enter = menuFlashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
        .showval = NULL,
        .edit = NULL,
        .hold = pwrOff,
    },
};
static ViewMenuStatic vMenuPower(menupower, sizeof(menupower)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню тестирования оборудования
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuhwtest[] {
    {
        .name = PSTR("Clock"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            sprintf_P(txt, PSTR("%d:%02d:%02d"), tmNow().h, tmNow().m, tmNow().s);
        },
    },
#if HWVER > 1
    {
        .name = PSTR("Battery"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[10];
            uint16_t bval = pwrBattValue();
            float vpin = 3.35 * bval / 4095;
            
            valOk(ok, (bval > 2400) && (bval < 3450));
            sprintf_P(txt, PSTR("(%0.2fv) %s"), vpin * 3 / 2, ok);
        },
    },
#endif
#if HWVER >= 3
    {
        .name = PSTR("Batt charge"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valYes(txt, pwrBattCharge()); },
    },
#endif
    {
        .name = PSTR("Pressure"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[10];
            float press = altCalc().press();
            
            valOk(ok, (press > 60000) && (press < 150000));
            sprintf_P(txt, PSTR("(%0.0fkPa) %s"), press / 1000, ok);
        },
    },
    {
        .name = PSTR("Light test"),
        .submenu = NULL,
        .enter = [] () {
            displayLightTgl();
            delay(2000);
            displayLightTgl();
        },
        .showval = [] (char *txt) { valOn(txt, displayLight()); },
    },
    {
        .name = PSTR("GPS data"),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[10];
            uint32_t dage = gpsDataAge();
            
            valOk(ok, dage < 250);
            if (dage < 1000)
                sprintf_P(txt, PSTR("(%d ms) %s"), dage, ok);
            else
            if (dage < 30000)
                sprintf_P(txt, PSTR("(%d s) fail"), dage / 1000);
            else
                strcpy_P(txt, PSTR("no data"));
        },
    },
#if HWVER > 1
    {
        .name = PSTR("GPS power restart"),
        .submenu = NULL,
        .enter = [] () {
            digitalWrite(HWPOWER_PIN_GPS, HIGH);
            delay(1000);
            digitalWrite(HWPOWER_PIN_GPS, LOW);
            menuFlashP(PSTR("GPS restarted"), 10);
        },
    },
#endif
    {
        .name = PSTR("GPS init"),
        .submenu = NULL,
        .enter = [] () {
            gpsInit();
            menuFlashP(PSTR("GPS inited"), 10);
        },
    },
};
static ViewMenuStatic vMenuHwTest(menuhwtest, sizeof(menuhwtest)/sizeof(menu_el_t), true);

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления остальными системными настройками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menusystem[] {
    {
        .name = PSTR("Factory reset"),
        .submenu = NULL,
        .enter = menuFlashHold,     // Сброс настроек только по длинному нажатию
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            menuFlashP(PSTR("formating eeprom"));
            displayUpdate(); // чтобы сразу вывести текст на экран
            if (!cfgFactory()) {
                menuFlashP(PSTR("EEPROM fail"));
                return;
            }
            menuFlashP(PSTR("Config reseted"));
            //displayUpdate(); // чтобы сразу вывести текст на экран
            //ESP.restart();
        },
    },
    {
        .name = PSTR("GPS serial"),
        .submenu = NULL,
        .enter = gpsDirectTgl,
        .showval = [] (char *txt) { valOn(txt, gpsDirect()); },
    },
    {
        .name       = PSTR("Hardware test"),
        .submenu    = &vMenuHwTest,
    },
};
static ViewMenuStatic vMenuSystem(menusystem, sizeof(menusystem)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Внешние подменю
 * ------------------------------------------------------------------------------------------- */
ViewMenu *menuLogBook();
ViewMenu *menuFile();
ViewMenu *menuWifiSync();

/* ------------------------------------------------------------------------------------------- *
 *  Главное меню конфига, тут в основном только подразделы
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menumain[] {
    {
        .name       = PSTR("GPS points"),
        .submenu    = &vMenuGpsPoint,
    },
    {
        .name       = PSTR("Jump count"),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valInt(txt, jmp.d().count); },
        .edit       = [] (int val) {
            int32_t c = jmp.d().count + val;
            if (c < 0) c = 0;
            if (c == jmp.d().count) return;
            jmp.set().count = c;
        },
    },
    {
        .name       = PSTR("LogBook"),
        .submenu    = menuLogBook(),
    },
    {
        .name       = PSTR("Track"),
        .submenu    = &vMenuTrack,
    },
    {
        .name       = PSTR("Display"),
        .submenu    = &vMenuDisplay,
    },
    {
        .name       = PSTR("Gnd Correct"),
        .submenu    = &vMenuGnd,
    },
    {
        .name       = PSTR("Auto Screen-Mode"),
        .submenu    = &vMenuInfo,
    },
    {
        .name       = PSTR("Time"),
        .submenu    = &vMenuTime,
    },
    {
        .name       = PSTR("Power"),
        .submenu    = &vMenuPower,
    },
    {
        .name       = PSTR("System"),
        .submenu    = &vMenuSystem,
    },
    {
        .name       = PSTR("Files"),
        .submenu    = menuFile(),
    },
    {
        .name       = PSTR("Wifi sync"),
        .submenu    = menuWifiSync(),
    },
};
static ViewMenuStatic vMenuMain(menumain, sizeof(menumain)/sizeof(menu_el_t));
void setViewMenu() {
    viewSet(vMenuMain);
    vMenuMain.open(NULL, PSTR("Configuration"));
}
