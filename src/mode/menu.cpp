
#include "../mode.h"
#include "../display.h"
#include "../button.h"
#include "../gps.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../altimeter.h"
#include "../timer.h"
#include "../../def.h" //time

// Заголовки функций, которые нужны в описании списка меню
static void valInt(char *txt, int val);
static void valOn(char *txt, bool val);
static void valYes(char *txt, bool val);
static void valDsplAuto(char *txt, int8_t val);
static void flashP(char *txt, int16_t count = 20);
static void flashHold();
static void flashClear();

static void menuExit();
static void menuSubMain(int16_t sel);
static void menuSubPoint();
static void menuSubDisplay();
static void menuSubGnd();
static void menuSubInfo();
static void menuSubTime();
static void menuSubPower();
static void menuSubSystem();
static void menuSubWifi();


/* *********************************************************************
 *      Элементы меню представлены в виде списка, каждый из которых
 *      описывает, что с ним можно сделать
 *
 *      Для простоты все операции проделываются через обработчики,
 *      исключение только у режима редактирования - он описан
 *      обработчиками editup и editdown, а вход в него происходит
 *      по нажатию на среднюю кнопку, если описан обработчик showval,
 *      т.е. если есть значение, которое можно изменить.
 *
 *      В остальном всё управляется через обработчики enter (короткое
 *      нажатие на среднюю кнопку) или hold (длинное на среднюю).
 *
 *      Для хождения по подменю существуют функции menuSubХХХ,
 *      их надо прописать в обработчик enter нужных пунктов меню
 *      или пункта Exit - для этого нет штатных инструментов.
 * *********************************************************************/

/* ------------------------------------------------------------------------------------------- *
 *  Главоне меню конфига, тут в основном только подразделы
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menumain[] {
    {
        .name = PSTR("Exit"),
        .enter = menuExit,
    },
    {
        .name = PSTR("GPS points"),
        .enter = menuSubPoint,
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
        .name = PSTR("Display"),
        .enter = menuSubDisplay,
    },
    {
        .name = PSTR("Gnd Correct"),
        .enter = menuSubGnd,
    },
    {
        .name = PSTR("Auto Screen-Mode"),
        .enter = menuSubInfo,
    },
    {
        .name = PSTR("Time"),
        .enter = menuSubTime,
    },
    {
        .name = PSTR("Power"),
        .enter = menuSubPower,
    },
    {
        .name = PSTR("System"),
        .enter = menuSubSystem,
    },
    {
        .name = PSTR("Wifi sync"),
        .enter = menuSubWifi,
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления GPS-точками
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menugpsupoint[] {
    {
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(1); },
    },
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
        .enter = flashHold, // Сохранение только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                flashP(PSTR("Point not selected"));
                return;
            }
            
            TinyGPSPlus &gps = gpsGet();
            if ((gps.satellites.value() == 0) || !gps.location.isValid()) {
                // Или к моменту срабатывания длинного нажатия может не быть валидных координат (потеряны спутники)
                flashP(PSTR("GPS not valid"));
                return;
            }
            
            // Сохраняем
            pnt.locSet(gps.location.lat(), gps.location.lng());
            if (!pnt.save()) {
                flashP(PSTR("EEPROM fail"));
                return;
            }
            
            flashP(PSTR("Point saved"));
        },
    },
    {   // Стирание координат у выбранной точки
        .name = PSTR("Clear"),
        .enter = flashHold,  // Стирание только по длинному нажатию, чтобы случайно не затереть точку
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!pnt.numValid()) { // Точка может быть не выбрана
                flashP(PSTR("Point not selected"));
                return;
            }
            
            // Очищаем
            pnt.locClear();
            if (!pnt.save()) {
                flashP(PSTR("EEPROM fail"));
                return;
            }
            
            flashP(PSTR("Point cleared"));
        },
    },
};

/* ------------------------------------------------------------------------------------------- *
 *  Меню управления экраном
 * ------------------------------------------------------------------------------------------- */
static const menu_el_t menudisplay[] {
    {
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(2); },
    },
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
    {
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(3); },
    },
    {   // принудительная калибровка (On Ground)
        .name = PSTR("Manual set"),
        .enter = flashHold,  // Сброс только по длинному нажатию
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!cfg.d().gndmanual) {
                flashP(PSTR("Manual not allowed"));
                return;
            }
            
            altCalc().gndreset();
            flashP(PSTR("GND corrected"));
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
    {
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(4); },
    },
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
    {
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(5); },
    },
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
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(6); },
    },
    {
        .name = PSTR("Off"),
        .enter = flashHold,     // Отключение питания только по длинному нажатию, чтобы не выключить случайно
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
        .name = PSTR("Exit"),
        .enter = [] () { menuSubMain(7); },
    },
    {
        .name = PSTR("Factory reset"),
        .enter = flashHold,     // Сброс настроек только по длинному нажатию
        .showval = NULL,
        .editup = NULL,
        .editdown = NULL,
        .hold = [] () {
            if (!cfgFactory()) {
                flashP(PSTR("EEPROM fail"));
                return;
            }
            flashP(PSTR("Config reseted"));
            ESP.restart();
        },
    },
};

// текущий список пунктов меню для отрисовки и перелистывания
static const menu_el_t *menu = NULL;
// индекс выбранного пункта меню и самого верхнего видимого на экране, а так же полное количество элементов
static int16_t menusel=0, menutop=0, menusz=0;
// Заголовок (в самом верху экрана)
static char *menutitle = PSTR("");
// редактируется ли текущий пункт меню
static bool menuedit = false;
// Сообщение моргающим текстом - будет выводится на селекторе меню (в этот момент сам выделенный пункт меню скрывается)
static char flashstr[20] = { '\0' };
static int16_t flashcnt = 0; // счётчик мигания - это и таймер мигания, и помогает мигать

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayMenu(U8G2 &u8g2) {
    char s[20]; // строка для вытаскивания строк из PROGMEM
    static uint8_t valflash = 0; // В режиме редактирования помогает мигать отображению значения
    
    if (menu == NULL) return;
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    u8g2.drawBox(0,0,128,12);
    strcpy_P(s, menutitle);
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(menusel-menutop)*10+14,128,10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        uint8_t i = n + menutop;    // i - индекс отрисовываемого пункта меню
        if (i >= menusz) break;     // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        u8g2.setDrawColor(menusel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;   // координата y для отрисовки текста (в нескольких местах используется)
        
        if ((flashcnt > 0) && (menusel == i)) {
            // если нужно вывести моргающее сообщение, делаем это вместо отрисовки
            // текстовой части этого пункта меню.
            // Внимание зрителя в этот момент как раз сосредоточено на этом пункте,
            // ему не надо будет бегать глазами по экрану в ругое место,
            // а текстовая часть самого меню в момент отображения мигающего сообщения как раз не нужна.
            if ((flashcnt--) % 4 >= 2) {
                // алгоритм мигания. При отрисовке каждые 200мс полный цикла потухания и зажигания равен 0.8 сек
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(flashstr))/2, y, flashstr);
            }
            continue;
        }
        
        const auto &m = menu[i];    // пункт меню, который надо сейчас отобразить
        strcpy_P(s, m.name);        // Название слева
        u8g2.drawStr(2, y, s);
        if (
                (m.showval != NULL) &&      // вывод значения
                (!menuedit || (i != menusel) || ((valflash++) % 4 >= 2)) // мерцание редактируемого значения
            ) {
            // выводим значение, если есть обработчик
            // в режиме редактирования - мигаем, и только выбранным пунктом меню
            m.showval(s);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s)-2, y, s);
        }
    }
    u8g2.setDrawColor(1);
}

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
 *  Функции мигания сообщением на выбранном пункте меню
 * ------------------------------------------------------------------------------------------- */
static void flashP(char *txt, int16_t count) {  // Запоминаем текст сообщения и сколько тактов отображения показывать
    strncpy_P(flashstr, txt, sizeof(flashstr));
    flashstr[sizeof(flashstr)] = '\0';
    flashcnt=count;
}

static void flashHold() {       // Стандартное сообщение о том, что для данной процедуры нужно удерживать
                                // среднюю кнопку три секунды
    flashP(PSTR("Hold select 3 sec"), 15);
}

static void flashClear() {      // Очистка моргающего сообщения в случае продолжения пользования меню
    flashstr[0] = '\0';
    flashcnt=0;
}


static void menuEditOn();  // Заголовок функции входа в режим редактирования нужно в функции menuHnd()

/* ------------------------------------------------------------------------------------------- *
 *  Обновление обработчиков средней кнопки при переключениями между меню, пунктами и режимами меню
 * ------------------------------------------------------------------------------------------- */
static void menuHnd() {
    const auto &m = menu[menusel];
    
    if ((m.showval != NULL) && ((m.editup != NULL) || (m.editdown != NULL)))
        // Если поле редактируемое
        btnHnd(BTN_SEL,     BTN_SIMPLE, menuEditOn);
    else if (m.enter != NULL)
        // Есть обработчик enter
        btnHnd(BTN_SEL,     BTN_SIMPLE, m.enter);
    else
        // Нет обработчика средней кнопки
        btnHnd(BTN_SEL,     BTN_SIMPLE, NULL);
    
    // Длинное нажатие есть у некоторых пунктов меню
    btnHnd(BTN_SEL,     BTN_LONG, m.hold != NULL ? m.hold : menuExit);
    
    flashClear();
    timerUpdate(MENU_TIMEOUT);  // Обновляем таймер автовыхода
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции перехода между пунктами меню
 * ------------------------------------------------------------------------------------------- */
static void menuUp() {      // Вверх
    menusel --;
    if (menusel >= 0) {
        if (menutop > menusel)  // если вылезли вверх за видимую область,
            menutop = menusel;  // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else { // если упёрлись в самый верх, перескакиваем в конец меню
        menusel = menusz-1;
        menutop = menusz > MENU_STR_COUNT ? menusz-MENU_STR_COUNT : 0;
    }
    
    menuHnd();
}

static void menuDown() {    // вниз
    menusel ++;
    if (menusel < menusz) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((menusel > menutop) && ((menusel-menutop) >= MENU_STR_COUNT))
            menutop = menusel-MENU_STR_COUNT+1;
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        menusel = 0;
        menutop=0;
    }
    
    menuHnd();
}

/* ------------------------------------------------------------------------------------------- *
 *  Изменения значения при редактировании
 * ------------------------------------------------------------------------------------------- */
static void editUp() {      // Вверх
    const auto &m = menu[menusel];
    if (m.editup == NULL) return;
    m.editup();
    flashClear();
    timerUpdate(MENU_TIMEOUT);  // Обновляем таймер автовыхода
}

static void editDown() {    // вниз
    const auto &m = menu[menusel];
    if (m.editdown == NULL) return;
    m.editdown();
    flashClear();
    timerUpdate(MENU_TIMEOUT);  // Обновляем таймер автовыхода
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции режима редактирования
 * ------------------------------------------------------------------------------------------- */
static void menuEditOff() {     // Выход из режима редактирования
    menuedit = false;   // возвращаем обратно определения кнопок
    btnHnd(BTN_UP,      BTN_SIMPLE, menuUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, menuDown);
    menuHnd();
}

static void menuEditOn() {      // Вход в режим редактирования
    const auto &m = menu[menusel];
    
    if ((m.showval == NULL) || ((m.editup == NULL) && (m.editdown == NULL)))
        // доп контроль, что это редактируемое поле
        return;
    
    // Переназначаем кнопки
    btnHnd(BTN_UP,      BTN_SIMPLE, editUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, editDown);
    btnHnd(BTN_SEL,     BTN_SIMPLE, menuEditOff);
    
    if (m.enter != NULL)    // При входе в режим редактирования можно использовать обработчик enter (как и при обычном нажатии)
        m.enter();
    
    menuedit = true;
    flashClear();
    timerUpdate(MENU_TIMEOUT);  // Обновляем таймер автовыхода
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции отображения разных меню
 * ------------------------------------------------------------------------------------------- */
static void menuSubMain(int16_t sel) {      // главное меню
    menu = menumain;
    menutitle = PSTR("Configuration");
    menusz = sizeof(menumain) / sizeof(menu_el_t);
    menusel=sel < menusz ? sel : 0;
    menutop=menusel >= MENU_STR_COUNT ? menusel+1-MENU_STR_COUNT : 0;
    menuHnd();
}

static void menuSubPoint() {
    menu = menugpsupoint;
    menutitle = PSTR("GPS points");
    menusz = sizeof(menugpsupoint) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubDisplay() {
    menu = menudisplay;
    menutitle = PSTR("Display");
    menusz = sizeof(menudisplay) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubGnd() {
    menu = menugnd;
    menutitle = PSTR("Gnd Correct");
    menusz = sizeof(menugnd) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubInfo() {
    menu = menuinfo;
    menutitle = PSTR("Auto Screen-Mode");
    menusz = sizeof(menuinfo) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubTime() {
    menu = menutime;
    menutitle = PSTR("Time");
    menusz = sizeof(menutime) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubPower() {
    menu = menupower;
    menutitle = PSTR("Power");
    menusz = sizeof(menupower) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}

static void menuSubSystem() {
    menu = menusystem;
    menutitle = PSTR("System");
    menusz = sizeof(menusystem) / sizeof(menu_el_t);
    menusel=0;
    menutop=0;
    menuHnd();
}


static void menuSubWifi() {
    // Сохраняем настройки, если изменены
    if (!cfgSave()) {
        flashP(PSTR("Config save error"));
        return;
    }
    timerClear();   // Убираем таймер автовыхода
    modeWifi();
}

/* ------------------------------------------------------------------------------------------- *
 *  Автопереключение главного экрана при смене режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_state_t prev, ac_state_t curr) {
    // Из меню мы только контролируем авто переход в режим FF
    if ((curr == ACST_FREEFALL) && cfg.d().dsplautoff)
        modeFF();
    // Остальные автопереключения при нахождении в меню нас не интересуют
}

/* ------------------------------------------------------------------------------------------- *
 *  Вход в меню
 * ------------------------------------------------------------------------------------------- */
void modeMenu() {
    modemain = false;
    displayHnd(displayMenu);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_UP,      BTN_SIMPLE, menuUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, menuDown);
    btnHnd(BTN_SEL,     BTN_LONG,   menuExit);
    altStateHnd(altState);
    
    timerHnd(menuExit, MENU_TIMEOUT); // Запускаем таймер автовыхода из меню
    
    menuSubMain(0);             // Отображение главного меню
    
    Serial.println(F("mode menu"));
}

/* ------------------------------------------------------------------------------------------- *
 *  Выход из меню
 * ------------------------------------------------------------------------------------------- */
static void menuExit() {
    // Сохраняем настройки, если изменены
    if (!cfgSave()) {
        flashP(PSTR("Config save error"));
        return;
    }
    timerClear();   // Убираем таймер автовыхода
    menu = NULL;
    menusel=0;
    menutop=0;
    menusz=0;
    menuedit = false;
    flashstr[0] = '\0';
    flashcnt = 0;
    modeMain();     // выходим в главный режим отображения показаний
}
