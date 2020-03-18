
#include "logbook.h"
#include "../logfile.h"
#include "../cfg/jump.h"

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса MenuStatic
 * ------------------------------------------------------------------------------------------- */
static size_t logSize() {
    return logRCountFull(PSTR(JMPLOG_SIMPLE_NAME), struct log_item_s<log_jmp_t>);
}

static int16_t ilog = 0;
static void logInfo() {
    modeLogBook(ilog);
}

MenuLogBook::MenuLogBook() :
    MenuBase(logSize(), PSTR("LogBook"))
{
}

void MenuLogBook::updStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuStatic::updStr: %d\r\n", i);
    
    struct log_item_s<log_jmp_t> r;
    if (logRead(r, PSTR(JMPLOG_SIMPLE_NAME), i)) {
        auto &dt = r.data.dt;
        snprintf_P(str.name, sizeof(str.name), PSTR("%2d.%02d.%02d %2d:%02d"),
                        dt.d, dt.m, dt.y % 100, dt.hh, dt.mm);
        snprintf_P(str.val, sizeof(str.val), PSTR("%d"), r.data.num);
    }
    else {
        str.name[0] = '\0';
        str.val[0] = '\0';
    }
}

void MenuLogBook::updHnd(int16_t i, button_hnd_t &smp, button_hnd_t &lng, button_hnd_t &editup, button_hnd_t &editdn) {
    ilog = i;
    smp = logInfo;
}
