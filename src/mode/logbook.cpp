
#include "../mode.h"
#include "../display.h"
#include "../button.h"
#include "../logfile.h"
#include "../cfg/jump.h"

static void logbookExit();

// индекс выбранного пункта меню и самого верхнего видимого на экране
static int16_t isel=0, itop=0, isz = 0;

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayLogBook(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    char s[20];
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,0,128,12);
    strcpy_P(s, PSTR("wifi srch"));
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(isel-itop)*10+14,128,10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        uint8_t i = n + itop;    // i - индекс отрисовываемого пункта меню
        if (i >= isz) break;     // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        u8g2.setDrawColor(isel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;   // координата y для отрисовки текста (в нескольких местах используется)
        
        // пункт меню, который надо сейчас отобразить
        snprintf_P(s, sizeof(s), PSTR("- %02d"), i);
        u8g2.drawStr(2, y, s);
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции перехода между пунктами меню
 * ------------------------------------------------------------------------------------------- */
static void logbookUp() {      // Вверх
    isel --;
    if (isel >= 0) {
        if (itop > isel)  // если вылезли вверх за видимую область,
            itop = isel;  // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else { // если упёрлись в самый верх, перескакиваем в конец меню
        isel = isz-1;
        itop = isz > MENU_STR_COUNT ? isz-MENU_STR_COUNT : 0;
    }
}

static void logbookDown() {    // вниз
    isel ++;
    if (isel < isz) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((isel > itop) && ((isel-itop) >= MENU_STR_COUNT))
            itop = isel-MENU_STR_COUNT+1;
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        isel = 0;
        itop=0;
    }
}

static void logbookSelect() {    // вниз
    if ((isel < 0) || (isel >= isz))
        return;
}

/* ------------------------------------------------------------------------------------------- *
 *  Вход в меню
 * ------------------------------------------------------------------------------------------- */
void modeLogBook() {
    modemain = false;
    displayHnd(displayLogBook);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_UP,      BTN_SIMPLE, logbookUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, logbookDown);
    btnHnd(BTN_SEL,     BTN_SIMPLE, logbookSelect);
    btnHnd(BTN_SEL,     BTN_LONG,   logbookExit);
    
    Serial.println(F("mode logbook"));
    
    isz = logRCountFull(PSTR(JMPLOG_SIMPLE_NAME), struct log_item_s<log_jmp_t>);
    Serial.println(isz);
}

/* ------------------------------------------------------------------------------------------- *
 *  Выход из меню
 * ------------------------------------------------------------------------------------------- */
static void logbookExit() {
    isel=0;
    itop=0;
    modeMain();     // выходим в главный режим отображения показаний
}
