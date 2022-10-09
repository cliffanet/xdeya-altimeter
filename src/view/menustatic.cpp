
#include "menu.h"
#include "info.h"
#include "../log.h"
#include "../power.h"
#include "../clock.h"
#include "../view/base.h"
#include "../navi/proc.h"
#include "../navi/compass.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../cfg/jump.h"
#include "../jump/proc.h"
#include "../jump/track.h"
#include "../core/filetxt.h"
#include "../../def.h" // time/pwr

#if HWVER >= 5
#include "../core/file.h"
#include <SD.h>
#endif


typedef void (*menu_hnd_t)();
typedef void (*menu_edit_t)(int val);
typedef void (*menu_val_t)(char *txt);

// Элемент меню
typedef struct {
    const char  *name;      // Текстовое название пункта
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
                    menuFlashP(PTXT(MENU_CONFIG_SAVEERROR));
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
 *  Внешние подменю
 * ------------------------------------------------------------------------------------------- */
ViewMenu *menuLogBook();
ViewMenu *menuWifiSync();
#if HWVER >= 5
ViewMenu *menuFwSdCard();
#endif
void setViewMagCalib();

/* ------------------------------------------------------------------------------------------- *
 *  Функции стандартного текстового вывода значения
 * ------------------------------------------------------------------------------------------- */
static void valInt(char *txt, int val) {    // числовые
    sprintf_P(txt, PSTR("%d"), val);
}
static void valOn(char *txt, bool val) {    // On/Off
    strcpy_P(txt, val ? PTXT(MENU_ON) : PTXT(MENU_OFF));
}
static void valYes(char *txt, bool val) {   // Yes/No
    strcpy_P(txt, val ? PTXT(MENU_YES) : PTXT(MENU_NO));
}
static void valOk(char *txt, bool val) {    // On/Off
    strcpy_P(txt, val ? PTXT(MENU_OK) : PTXT(MENU_FAIL));
}
static void valDsplAuto(char *txt, int8_t val) {
    switch (val) {
        case MODE_MAIN_NONE:    strcpy_P(txt, PTXT(MENU_MAINPAGE_NONE)); break;
        case MODE_MAIN_LAST:    strcpy_P(txt, PTXT(MENU_MAINPAGE_LAST)); break;
        case MODE_MAIN_ALTNAV:  strcpy_P(txt, PTXT(MENU_MAINPAGE_ALTNAV)); break;
        case MODE_MAIN_ALT:     strcpy_P(txt, PTXT(MENU_MAINPAGE_ALT)); break;
        case MODE_MAIN_NAV:     strcpy_P(txt, PTXT(MENU_MAINPAGE_NAV)); break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления NAVI-точками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugpsupoint[] {
    {   // Выбор текущей точки
        .name = PTXT(MENU_POINT_CURRENT),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            if (pnt.numValid()) {
                valInt(txt, pnt.num());
                if (!pnt.cur().used)
                    strcpy_P(txt+strlen(txt), PTXT(MENU_POINT_NOTUSED));
            }
            else
                strcpy_P(txt, PTXT(MENU_POINT_NOCURR));
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
        .name = PTXT(MENU_POINT_SAVE),
        .submenu = NULL,
        .enter = menuFlashHold, // Сохранение только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                menuFlashP(PTXT(MENU_POINT_NOTSELECT));
                return;
            }
            
            auto &gps = gpsInf();
            if (!NAV_VALID_LOCATION(gps)) {
                // Или к моменту срабатывания длинного нажатия может не быть валидных координат (потеряны спутники)
                menuFlashP(PTXT(MENU_POINT_NONAV));
                return;
            }
            
            // Сохраняем
            pnt.locSet(NAV_LATLON(gps.lat), NAV_LATLON(gps.lon));
            if (!pnt.save()) {
                menuFlashP(PTXT(MENU_POINT_EEPROMFAIL));
                return;
            }
            
            menuFlashP(PTXT(MENU_POINT_SAVED));
        },
    },
    {   // Стирание координат у выбранной точки
        .name = PTXT(MENU_POINT_CLEAR),
        .submenu = NULL,
        .enter = menuFlashHold,  // Стирание только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                menuFlashP(PTXT(MENU_POINT_NOTSELECT));
                return;
            }
            
            // Очищаем
            pnt.locClear();
            if (!pnt.save()) {
                menuFlashP(PTXT(MENU_POINT_EEPROMFAIL));
                return;
            }
            
            menuFlashP(PTXT(MENU_POINT_CLEARED));
        },
    },
};
static ViewMenuStatic vMenuGpsPoint(menugpsupoint, sizeof(menugpsupoint)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления экраном
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menudisplay[] {
    {   // Включение / выключение подсветки
        .name = PTXT(MENU_DISPLAY_LIGHT),
        .submenu = NULL,
        .enter = displayLightTgl,           // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) { valOn(txt, displayLight()); },
    },
    {   // Уровень контраста
        .name = PTXT(MENU_DISPLAY_CONTRAST),
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
    {   // Переворот на 180
        .name = PTXT(MENU_DISPLAY_FLIPP180),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().flipp180 = !cfg.d().flipp180;
            displayFlipp180(cfg.d().flipp180);
            btnFlipp180(cfg.d().flipp180);
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().flipp180); },
    },
};
static ViewMenuStatic vMenuDisplay(menudisplay, sizeof(menudisplay)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню сброса нуля высотомера (на земле)
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugnd[] {
    {   // принудительная калибровка (On Ground)
        .name = PTXT(MENU_GND_MANUALSET),
        .submenu = NULL,
        .enter = menuFlashHold,  // Сброс только по длинному нажатию
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            if (!cfg.d().gndmanual) {
                menuFlashP(PTXT(MENU_GND_MANUALNOTALLOW));
                return;
            }
            
            altCalc().gndreset();
            jmpReset();
            menuFlashP(PTXT(MENU_GND_CORRECTED));
        },
    },
    {   // разрешение принудительной калибровки: нет, "на земле", всегда
        .name = PTXT(MENU_GND_MANUALALLOW),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().gndmanual = !cfg.d().gndmanual;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().gndmanual); },
    },
    {   // выбор автоматической калибровки: нет, автоматически на земле
        .name = PTXT(MENU_GND_AUTOCORRECT),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().gndauto = !cfg.d().gndauto;
        },
        .showval = [] (char *txt) { strcpy_P(txt, cfg.d().gndauto ? PTXT(MENU_GND_CORRECTONGND) : PTXT(MENU_NO)); },
    },
    {   // превышение площадки приземления
        .name       = PTXT(MENU_GND_ALTCORRECT),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            if (cfg.d().altcorrect == 0)
                strcpy_P(txt, PTXT(MENU_DISABLE));
            else
                valInt(txt, cfg.d().altcorrect);
        },
        .edit       = [] (int val) {
            int32_t c = cfg.d().altcorrect;
            c += val;
            if (c < -6000) c = -6000;
            if (c > 6000) c = 6000;
            if (c == cfg.d().altcorrect) return;
            cfg.set().altcorrect = c;
        },
    },
};
static ViewMenuStatic vMenuGnd(menugnd, sizeof(menugnd)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню автоматизации экранами с информацией
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuinfo[] {
    {   // переключать ли экран в падении автоматически в отображение только высоты
        .name = PTXT(MENU_AUTOMAIN_FF),
        .submenu = NULL,
        .enter = [] () {
            cfg.set().dsplautoff = !cfg.d().dsplautoff;
        },
        .showval = [] (char *txt) { valYes(txt, cfg.d().dsplautoff); },
    },
    {   // куда переключать экран под куполом: не переключать, жпс, часы, жпс+высота (по умолч)
        .name = PTXT(MENU_AUTOMAIN_CNP),
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
        .name = PTXT(MENU_AUTOMAIN_LAND),
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
        .name = PTXT(MENU_AUTOMAIN_ONGND),
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
        .name = PTXT(MENU_AUTOMAIN_POWERON),
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
 *  Меню автоотключения питания
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menupoweroff[] {
    {
        .name = PTXT(MENU_POWEROFF_FORCE),
        .submenu = NULL,
        .enter = menuFlashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
        .showval = NULL,
        .edit = NULL,
        .hold = pwrOff,
    },
    {   // отключаться через N часов отсутствия взлётов
        .name       = PTXT(MENU_POWEROFF_NOFLY),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            if (cfg.d().hrpwrnofly > 0)
                valInt(txt, cfg.d().hrpwrnofly);
            else
                strcpy_P(txt, PTXT(MENU_DISABLE));
        },
        .edit       = [] (int val) {
            int32_t c = cfg.d().hrpwrnofly;
            c += val;
            if (c < 0) c = 0;
            if (c > 24) c = 24;
            if (c == cfg.d().hrpwrnofly) return;
            cfg.set().hrpwrnofly = c;
        },
    },
    {   // отключаться через N часов после включения
        .name       = PTXT(MENU_POWEROFF_AFTON),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            if (cfg.d().hrpwrafton > 0)
                valInt(txt, cfg.d().hrpwrafton);
            else
                strcpy_P(txt, PTXT(MENU_DISABLE));
        },
        .edit       = [] (int val) {
            int32_t c = cfg.d().hrpwrafton;
            c += val;
            if (c < 0) c = 0;
            if (c > 24) c = 24;
            if (c == cfg.d().hrpwrafton) return;
            cfg.set().hrpwrafton = c;
        },
    },
};
static ViewMenuStatic vMenuPowerOff(menupoweroff, sizeof(menupoweroff)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню автоматического включения NAV-приёмника
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugpson[] {
    {   // Включение / выключение питания NAVI вручную
        .name = PTXT(MENU_NAVON_CURRENT),
        .submenu = NULL,
        .enter = gpsPwrTgl,                 // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) { valOn(txt, gpsPwr()); },
    },
    {   // авто-включение сразу при включении питания
        .name       = PTXT(MENU_NAVON_POWERON),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valYes(txt, cfg.d().navonpwron); },
        .edit       = [] (int val) { cfg.set().navonpwron = !cfg.d().navonpwron; },
    },
    {   // автоматически включать при старте записи трека
        .name       = PTXT(MENU_NAVON_TRKREC),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valYes(txt, cfg.d().navontrkrec); },
        .edit       = [] (int val) { cfg.set().navontrkrec = !cfg.d().navontrkrec; },
    },
    {   // авто-включение в подъёме на заданной высоте
        .name       = PTXT(MENU_NAVON_TAKEOFF),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            if (cfg.d().navonalt > 0)
                valInt(txt, cfg.d().navonalt);
            else
                strcpy_P(txt, PTXT(MENU_DISABLE));
        },
        .edit       = [] (int val) {
            int32_t c = cfg.d().navonalt;
            c += val*10;
            if (c < 0) c = 0;
            if (c > 6000) c = 6000;
            if (c == cfg.d().navonalt) return;
            cfg.set().navonalt = c;
        },
    },
    {   // авто-отключать жпс всегда после приземления
        .name       = PTXT(MENU_NAVON_OFFLAND),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valYes(txt, cfg.d().navoffland); },
        .edit       = [] (int val) { cfg.set().navoffland = !cfg.d().navoffland; },
    },
    {   // Режим GPS/GLONASS
        .name       = PTXT(MENU_NAV_MODE),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            switch (cfg.d().navmode) {
                case 0:
                case 3: strcpy_P(txt, PTXT(MENU_NAV_MODE_GLONGPS)); break;
                case 1: strcpy_P(txt, PTXT(MENU_NAV_MODE_GLONASS)); break;
                case 2: strcpy_P(txt, PTXT(MENU_NAV_MODE_GPS)); break;
                default: *txt = 0;
            }
        },
        .edit       = [] (int val) {
            if (val > 0) {
                cfg.set().navmode =
                        cfg.d().navmode >= 3 ?
                            1 : cfg.d().navmode+1;
            }
            else
            if (val < 0) {
                cfg.set().navmode =
                        cfg.d().navmode <= 1 ?
                            3 : cfg.d().navmode-1;
            }
            gpsUpdateMode();
        },
    },
};
static ViewMenuStatic vMenuGpsOn(menugpson, sizeof(menugpson)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню выбора функций кнопок вверх/вниз
 * ------------------------------------------------------------------------------------------- */
static void valBtnDo(char *txt, uint8_t op) {
    switch (op) {
        case BTNDO_NONE:
            strcpy_P(txt, PTXT(MENU_BTNDO_NO));
            break;
            
        case BTNDO_LIGHT:
            strcpy_P(txt, PTXT(MENU_BTNDO_BACKLIGHT));
            break;
            
        case BTNDO_NAVPWR:
            strcpy_P(txt, PTXT(MENU_BTNDO_NAVTGL));
            break;
            
        case BTNDO_TRKREC:
            strcpy_P(txt, PTXT(MENU_BTNDO_TRKREC));
            break;
            
        case BTNDO_PWROFF:
            strcpy_P(txt, PTXT(MENU_BTNDO_POWEROFF));
            break;
    }
}
static const menu_el_t menubtndo[] {
    {   // действие по кнопке "вверх"
        .name       = PTXT(MENU_BTNDO_UP),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valBtnDo(txt, cfg.d().btndo_up); },
        .edit       = [] (int val) {
            cfg.set().btndo_up = val > 0 ? (
                    cfg.d().btndo_up < (BTNDO_MAX-1) ?
                        cfg.d().btndo_up+1 :
                        BTNDO_NONE
                ) : (
                    cfg.d().btndo_up > BTNDO_NONE ?
                        cfg.d().btndo_up-1 :
                        BTNDO_MAX-1
                );
        },
    },
    {   // действие по кнопке "вниз"
        .name       = PTXT(MENU_BTNDO_DOWN),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valBtnDo(txt, cfg.d().btndo_down); },
        .edit       = [] (int val) {
            cfg.set().btndo_down = val > 0 ? (
                    cfg.d().btndo_down < (BTNDO_MAX-1) ?
                        cfg.d().btndo_down+1 :
                        BTNDO_NONE
                ) : (
                    cfg.d().btndo_down > BTNDO_NONE ?
                        cfg.d().btndo_down-1 :
                        BTNDO_MAX-1
                );
        },
    },
};
static ViewMenuStatic vMenuBtnDo(menubtndo, sizeof(menubtndo)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню работы с трэками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menutrack[] {
    {   // Текущий режим записи трэка
        .name = PTXT(MENU_TRACK_RECORDING),
        .submenu = NULL,
        .enter = menuFlashHold,           // Переключаем в один клик без режима редактирования
        .showval = [] (char *txt) {
            if (trkRunning(TRK_RUNBY_HAND))
                strcpy_P(txt, PTXT(MENU_TRACK_FORCE));
            else
            if (trkRunning())
                strcpy_P(txt, PTXT(MENU_TRACK_AUTO));
            else
                strcpy_P(txt, PTXT(MENU_TRACK_NOREC));
        },
        .edit = NULL,
        .hold = [] () {
            if (trkRunning())
                trkStop();
            else
                trkStart(TRK_RUNBY_HAND);
        },
    },
    {   // автоматически включать трек на заданной высоте в подъёме
        .name       = PTXT(MENU_TRACK_BYALT),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) {
            if (cfg.d().trkonalt > 0)
                valInt(txt, cfg.d().trkonalt);
            else
                strcpy_P(txt, PTXT(MENU_DISABLE));
        },
        .edit       = [] (int val) {
            int32_t c = cfg.d().trkonalt;
            c += val*10;
            if (c < 0) c = 0;
            if (c > 6000) c = 6000;
            if (c == cfg.d().trkonalt) return;
            cfg.set().trkonalt = c;
        },
    },
    {   // Количество записей
        .name = PTXT(MENU_TRACK_COUNT),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valInt(txt, FileTrack().count()); },
    },
    {   // Сколько времени доступно
        .name = PTXT(MENU_TRACK_AVAIL),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            // сколько записей ещё влезет
            size_t avail = trkCountAvail() / 5; // количество секунд
            if (avail >= 60)
                sprintf_P(txt, PSTR("%d:%02d"), avail/60, avail%60);
            else
                sprintf_P(txt, PTXT(MENU_TRACK_AVAILSEC), avail);
        },
    },
};
static ViewMenuStatic vMenuTrack(menutrack, sizeof(menutrack)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления часами
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menutime[] {
    {   // Выбор временной зоны, чтобы корректно синхронизироваться с службами времени
        .name = PTXT(MENU_TIME_ZONE),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            if (cfg.d().timezone == 0) {                // cfg.timezone хранит количество минут в + или - от UTC
                strcpy_P(txt, PSTR("UTC"));             // при 0 - это время UTC
                return;
            }
            *txt = cfg.d().timezone > 0 ? '+' : '-';    // Отображение знака смещения
            txt++;
        
            uint16_t m = abs(cfg.d().timezone);         // в часах и минутах отображаем пояс
            txt += sprintf_P(txt, PSTR("%d"), m / 60);
            m = m % 60;
            sprintf_P(txt, PSTR(":%02d"), m);
        },
        .edit = [] (int val) {
            if (val == -1) {
                if (cfg.d().timezone >= 12*60)          // Ограничение выбора часового пояса
                    return;
                cfg.set().timezone += 30;               // часовые пояса смещаем по 30 минут
                clockForceAdjust();                     // сразу применяем настройки
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
 *  Меню опций
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuoptions[] {
    {
        .name       = PTXT(MENU_OPTION_DISPLAY),
        .submenu    = &vMenuDisplay,
    },
    {
        .name       = PTXT(MENU_OPTION_GNDCORRECT),
        .submenu    = &vMenuGnd,
    },
    {
        .name       = PTXT(MENU_OPTION_AUTOSCREEN),
        .submenu    = &vMenuInfo,
    },
    {
        .name       = PTXT(MENU_OPTION_AUTOPOWER),
        .submenu    = &vMenuPowerOff,
    },
    {
        .name       = PTXT(MENU_OPTION_AUTONAV),
        .submenu    = &vMenuGpsOn,
    },
    {   // использовать компас
        .name       = PTXT(MENU_OPTION_COMPEN),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valYes(txt, (cfg.d().flagen & FLAGEN_COMPAS) > 0); },
        .edit       = [] (int val) {
            if ((cfg.d().flagen & FLAGEN_COMPAS) == 0) {
                cfg.set().flagen |= FLAGEN_COMPAS;
                compInit();
            }
            else {
                cfg.set().flagen &= ~FLAGEN_COMPAS;
                compStop();
            }
        },
    },
    {   // Курс цифрами
        .name       = PTXT(MENU_OPTION_TXTCOURSE),
        .submenu    = NULL,
        .enter      = NULL,
        .showval    = [] (char *txt) { valYes(txt, (cfg.d().flagen & FLAGEN_TXTCOURSE) > 0); },
        .edit       = [] (int val) {
            if ((cfg.d().flagen & FLAGEN_TXTCOURSE) == 0)
                cfg.set().flagen |= FLAGEN_TXTCOURSE;
            else
                cfg.set().flagen &= ~FLAGEN_TXTCOURSE;
        },
    },
    {
        .name       = PTXT(MENU_OPTION_BTNDO),
        .submenu    = &vMenuBtnDo,
    },
    {
        .name       = PTXT(MENU_OPTION_TRACK),
        .submenu    = &vMenuTrack,
    },
    {
        .name       = PTXT(MENU_OPTION_TIME),
        .submenu    = &vMenuTime,
    },
};
static ViewMenuStatic vMenuOptions(menuoptions, sizeof(menuoptions)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню Обновления прошивки
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menufirmware[] {
    {
        .name = PTXT(MENU_FW_VERSION),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            strcpy_P(txt, PSTR(TOSTRING(FWVER_NUM)));
        },
    },
    {
        .name = PTXT(MENU_FW_TYPE),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            strcpy_P(txt, PSTR(FWVER_TYPE_DSPL));
        },
    },
    {
        .name = PTXT(MENU_FW_HWREV),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            strcpy_P(txt, PSTR(TOSTRING(v0.HWVER)));
        },
    },
    {
        .name = PTXT(MENU_FW_UPDATETO),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            if (cfg.d().fwupdind <= 0)
                strcpy_P(txt, PTXT(MENU_FW_NOUPD));
            else {
                FileTxt f(PSTR(VERAVAIL_FILE));
                if (!f.seek2line(cfg.d().fwupdind-1) ||
                    (f.read_line(txt, MENUSZ_VAL) < 1))
                    *txt = '\0';
                f.close();
            }
        },
        .edit = [] (int val) {
            FileTxt f(PSTR(VERAVAIL_FILE));
            size_t count = f.line_count();
            f.close();
            
            if (val > 0) {
                cfg.set().fwupdind --;
                if (cfg.d().fwupdind < 0)
                    cfg.set().fwupdind = count;
            }
            else
            if (val < 0) {
                cfg.set().fwupdind ++;
                if (cfg.d().fwupdind > count)
                    cfg.set().fwupdind = 0;
            }
        },
    },
#if HWVER >= 5
    {
        .name       = PTXT(MENU_FW_SDCARD),
        .submenu    = menuFwSdCard(),
    },
#endif // HWVER >= 5
};
static ViewMenuStatic vMenuFirmWare(menufirmware, sizeof(menufirmware)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Меню тестирования оборудования
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menuhwtest[] {
    {
        .name = PTXT(MENU_TEST_CLOCK),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            sprintf_P(txt, PSTR("%d:%02d:%02d"), tmNow().h, tmNow().m, tmNow().s);
        },
    },
#if HWVER > 1
    {
        .name = PTXT(MENU_TEST_BATTERY),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];
            uint16_t bval = pwrBattRaw();
            valOk(ok, (bval > 2400) && (bval < 3450));
            sprintf_P(txt, PSTR("(%0.2fv) %s"), pwrBattValue(), ok);
        },
    },
#endif
#if HWVER >= 3
    {
        .name = PTXT(MENU_TEST_BATTCHARGE),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) { valYes(txt, pwrBattCharge()); },
    },
#endif
    {
        .name = PTXT(MENU_TEST_PRESSURE),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];
            float press = altCalc().press();
            
            valOk(ok, (press > 60000) && (press < 150000));
            sprintf_P(txt, PTXT(MENU_TEST_PRESSVAL), press / 1000, ok);
        },
    },
    {
        .name = PTXT(MENU_TEST_LIGHT),
        .submenu = NULL,
        .enter = [] () {
            displayLightTgl();
            delay(2000);
            displayLightTgl();
        },
        .showval = [] (char *txt) { valOn(txt, displayLight()); },
    },
    {
        .name = PTXT(MENU_TEST_COMPASS),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];

            valOk(ok, compass().ok==0x3);
            int n = sprintf_P(txt, PTXT(MENU_TEST_COMPVAL), compass().ok, ok);
        },
    },
    {
        .name = PTXT(MENU_TEST_NAVDATA),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            char ok[15];
            uint16_t dage = gpsDataAge();
            
            valOk(ok, gpsInf().rcvok);
            if (dage < 15)
                sprintf_P(txt, PTXT(MENU_TEST_DATADELAY), dage*NAV_TICK_INTERVAL, ok);
            else
                strcpy_P(txt, PTXT(MENU_TEST_NONAVDATA));
        },
    },
#if HWVER > 1
    {
        .name = PTXT(MENU_TEST_NAVRESTART),
        .submenu = NULL,
        .enter = [] () {
            gpsRestart();
            menuFlashP(PTXT(MENU_TEST_NAVRESTARTED), 10);
        },
    },
#endif
    {
        .name = PTXT(MENU_TEST_NAVINIT),
        .submenu = NULL,
        .enter = [] () {
            gpsInit();
            menuFlashP(PTXT(MENU_TEST_NAVINITED), 10);
        },
    },
#if HWVER >= 5
    {
        .name = PTXT(MENU_TEST_SDCART),
        .submenu = NULL,
        .enter = NULL,
        .showval = [] (char *txt) {
            CONSOLE("sdcard test beg");
            fileExtInit();
            
            char t[15];
            auto type = SD.cardType();
            switch (type) {
                case CARD_NONE:     strcpy_P(t, PSTR("none"));  break;
                case CARD_MMC:      strcpy_P(t, PSTR("MMC"));   break;
                case CARD_SD:       strcpy_P(t, PSTR("SD"));    break;
                case CARD_SDHC:     strcpy_P(t, PSTR("SDHC"));  break;
                case CARD_UNKNOWN:  strcpy_P(t, PSTR("UNK"));   break;
                default:            sprintf_P(t, PSTR("[%d]"), type);
            }
            
            if (type != CARD_NONE) {
                double sz = static_cast<double>(SD.cardSize());
                char s = 'b';
                if (sz > 1024) {
                    sz = sz / 1024;
                    s = 'k';
                    if (sz > 1024) {
                        sz = sz / 1024;
                        s = 'M';
                    }
                    if (sz > 1024) {
                        sz = sz / 1024;
                        s = 'G';
                    }
                }

                sprintf_P(txt, PSTR("%s / %0.1f %c"), t, sz, s);
            }
            else {
                strcpy_P(txt, "none");
            }
            fileExtStop();
            CONSOLE("sdcard test end");
        },
    },
#endif
};
static ViewMenuStatic vMenuHwTest(menuhwtest, sizeof(menuhwtest)/sizeof(menu_el_t), true);

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления остальными системными настройками
 * ------------------------------------------------------------------------------------------- */
ViewMenu *menuFile();

static const menu_el_t menusystem[] {
    {
        .name = PTXT(MENU_POWEROFF_FORCE),
        .submenu = NULL,
        .enter = menuFlashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
        .showval = NULL,
        .edit = NULL,
        .hold = pwrOff,
    },
    {
        .name = PTXT(MENU_SYSTEM_RESTART),
        .submenu = NULL,
        .enter = menuFlashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
        .showval = NULL,
        .edit = NULL,
        .hold = [] () { esp_restart(); },
    },
    {
        .name = PTXT(MENU_SYSTEM_FACTORYRES),
        .submenu = NULL,
        .enter = menuFlashHold,     // Сброс настроек только по длинному нажатию
        .showval = NULL,
        .edit = NULL,
        .hold = [] () {
            menuFlashP(PTXT(MENU_SYSTEM_FORMATROM));
            displayUpdate(); // чтобы сразу вывести текст на экран
            btnWaitRelease();   // Надо ожидать, пока будет отпущена кнопка,
                                // иначе, если сработает прерывание во время cfgFactory(),
                                // будет: Core  1 panic'ed (Cache disabled but cached memory region accessed)
            if (!cfgFactory()) {
                menuFlashP(PTXT(MENU_SYSTEM_EEPROMFAIL));
                return;
            }
            menuFlashP(PTXT(MENU_SYSTEM_CFGRESETED));
            //displayUpdate(); // чтобы сразу вывести текст на экран
            //ESP.restart();
        },
    },
    {
        .name       = PTXT(MENU_SYSTEM_FWUPDATE),
        .submenu    = &vMenuFirmWare,
    },
    {
        .name       = PTXT(MENU_SYSTEM_FILES),
        .submenu    = menuFile(),
    },
    {
        .name = PTXT(MENU_SYSTEM_NAVSERIAL),
        .submenu = NULL,
        .enter = gpsDirectTgl,
        .showval = [] (char *txt) { valOn(txt, gpsDirect()); },
    },
    {
        .name = PTXT(MENU_SYSTEM_NAVSATINFO),
        .submenu = NULL,
        .enter = setViewInfoSat,
    },
    {   // Холодный перезапуск навигации
        .name       = PTXT(MENU_SYSTEM_NAVCOLDRST),
        .submenu    = NULL,
        .enter      = menuFlashHold,
        .showval    = NULL,
        .edit       = NULL,
        .hold       =  [] () {
            if (!gpsColdRestart()) {
                menuFlashP(PTXT(MENU_SYSTEM_NAVRSTFAIL));
                return;
            }
            
            menuFlashP(PTXT(MENU_SYSTEM_NAVRSTOK));
        },
    },
    {
        .name       = PTXT(MENU_SYSTEM_HWTEST),
        .submenu    = &vMenuHwTest,
    },
    {
        .name       = PTXT(MENU_SYSTEM_MAGCALIB),
        .submenu    = NULL,
        .enter      = setViewMagCalib,
    },
};
static ViewMenuStatic vMenuSystem(menusystem, sizeof(menusystem)/sizeof(menu_el_t));

/* ------------------------------------------------------------------------------------------- *
 *  Главное меню конфига, тут в основном только подразделы
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menumain[] {
    {
        .name       = PTXT(MENU_ROOT_NAVPOINT),
        .submenu    = &vMenuGpsPoint,
    },
    {
        .name       = PTXT(MENU_ROOT_JUMPCOUNT),
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
        .name       = PTXT(MENU_ROOT_LOGBOOK),
        .submenu    = menuLogBook(),
    },
    {
        .name       = PTXT(MENU_ROOT_OPTIONS),
        .submenu    = &vMenuOptions,
    },
    {
        .name       = PTXT(MENU_ROOT_SYSTEM),
        .submenu    = &vMenuSystem,
    },
    {
        .name       = PTXT(MENU_ROOT_WIFISYNC),
        .submenu    = menuWifiSync(),
    },
};
static ViewMenuStatic vMenuMain(menumain, sizeof(menumain)/sizeof(menu_el_t));
void setViewMenu() {
    viewSet(vMenuMain);
    vMenuMain.open(NULL, PTXT(MENU_ROOT));
}
