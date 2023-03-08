
#include "info.h"

void ViewInfo::prnstr(const char *s, txtalgn_t algn) {
    int8_t y = (iprn+1)*9+1;

    switch (algn) {
        case TXT_LEFT:
            _u8g2->drawStr(0, y, s);
            break;
        
        case TXT_RIGHT:
            _u8g2->drawStr(_u8g2->getDisplayWidth()-_u8g2->getStrWidth(s)-2, y, s);
            break;
        
        case TXT_CENTER:
            _u8g2->drawStr((_u8g2->getDisplayWidth()-_u8g2->getStrWidth(s))/2, y, s);
            break;
    }
}

void ViewInfo::vprn(txtalgn_t algn, const char *s, va_list ap) {
    int len = vsnprintf_P(NULL, 0, s, ap);
    char str[len+1];
    
    vsnprintf_P(str, sizeof(str), s, ap);
    prnstr(str, algn);
}

void ViewInfo::prnl(const char *s, ...) {
    va_list ap;
    va_start (ap, s);
    vprn(TXT_LEFT, s, ap);
    va_end (ap);
}

void ViewInfo::prnr(const char *s, ...) {
    va_list ap;
    va_start (ap, s);
    vprn(TXT_RIGHT, s, ap);
    va_end (ap);
}

void ViewInfo::prnc(const char *s, ...) {
    va_list ap;
    va_start (ap, s);
    vprn(TXT_CENTER, s, ap);
    va_end (ap);
}


void ViewInfo::btnSmpl(btn_code_t btn) {
    switch (btn) {
        case BTN_UP:
            if (itop == 0)
                return;
            itop--;
            break;

        case BTN_DOWN:
            {
                const uint8_t scnt = title == NULL ? INFO_STR_COUNT : INFO_STR_COUNT-1;
                const uint16_t itopmax = sz > scnt ? sz-scnt : 0;
                if (itop < itopmax)
                    itop++;
                else
                if (itop > itopmax)
                    itop = itopmax;
            }
            break;
    }
}

void ViewInfo::draw(U8G2 &u8g2) {
    _u8g2 = &u8g2;
    iprn = 0;
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    if (title != NULL) {
        // Заголовок
        u8g2.drawBox(0,0,u8g2.getDisplayWidth(),10);
        u8g2.setDrawColor(0);
        
        char s[128];
        strncpy_P(s, title, sizeof(s));
        s[sizeof(s)-1] = '\0';
        u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 9, s);
        
        iprn ++;
    }
    
    u8g2.setDrawColor(1);

#if HWVER < 4
    const uint8_t strcnt = 7;
#else
    const uint8_t strcnt = 10;
#endif
    
    for (uint16_t i = itop; (i<sz) && (iprn<strcnt); i++, iprn++)
        updStr(i);
}
