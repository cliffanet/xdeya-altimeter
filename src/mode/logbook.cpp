
#include "../menu/base.h"
#include "../display.h"
#include "../button.h"
#include "../file/log.h"
#include "../cfg/jump.h"

// индекс выбранного пункта меню и самого верхнего видимого на экране
static size_t isel=0, sz=0;
static struct log_item_s<log_jmp_t> r;

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayLogBook(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    const auto &d = r.data;
    
    // Заголовок
    char s[20];
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,0,128,12);
    snprintf_P(s, sizeof(s), PSTR("Jump # %d"), d.num);
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
    u8g2.setDrawColor(1);
    int8_t y = 10-1+14;
    
    const auto &dt = d.dt;
    snprintf_P(s, sizeof(s), PSTR("%2d.%02d.%04d"), dt.d, dt.m, dt.y);
    u8g2.drawStr(0, y, s);
    snprintf_P(s, sizeof(s), PSTR("%2d:%02d"), dt.hh, dt.mm);
    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
    y += 10;
    strcpy_P(s, PSTR("Alt"));
    u8g2.drawStr(0, y, s);
    snprintf_P(s, sizeof(s), PSTR("%.0f"), d.beg.alt);
    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
    y += 10;
    strcpy_P(s, PSTR("Deploy"));
    u8g2.drawStr(0, y, s);
    snprintf_P(s, sizeof(s), PSTR("%.0f"), d.cnp.alt);
    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
    y += 10;
    strcpy_P(s, PSTR("FF time"));
    u8g2.drawStr(0, y, s);
    snprintf_P(s, sizeof(s), PSTR("%d s"), (d.cnp.mill-d.beg.mill)/1000);
    u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
}

static bool logbookRead() {
    return logRead(r, PSTR(JMPLOG_SIMPLE_NAME), isel);
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции перехода между пунктами меню
 * ------------------------------------------------------------------------------------------- */
static void btnUp() {      // Вверх
    if (isel <= 0)
        return;
    isel --;
    logbookRead();
}

static void btnDown() {    // вниз
    if ((isel+1) >= sz)
        return;
    isel ++;
    
    logbookRead();
}

/* ------------------------------------------------------------------------------------------- *
 *  Вход в меню
 * ------------------------------------------------------------------------------------------- */
void modeLogBook(size_t i) {
    displayHnd(displayLogBook);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_UP,      BTN_SIMPLE, btnUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, btnDown);
    btnHnd(BTN_SEL,     BTN_SIMPLE, modeMenu);
    
    Serial.println(F("mode logbook"));
    
    sz = logRCountFull(PSTR(JMPLOG_SIMPLE_NAME), struct log_item_s<log_jmp_t>);
    isel = i;
    logbookRead();
}
