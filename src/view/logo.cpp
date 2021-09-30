
#include "logo.h"
#include "main.h"
#include "../cfg/main.h"

void ViewLogo::draw(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_open_iconic_other_8x_t);
    u8g2.drawGlyph((u8g2.getDisplayWidth()-56)/2, (u8g2.getDisplayHeight()-35)/2 + 56/2, 0x40);
    
    char s[30];
    u8g2.setFont(u8g2_font_ImpactBits_tr);
    strcpy_P(s, PSTR("HYBRIDO"));
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, u8g2.getDisplayHeight()-20, s);
    strcpy_P(s, PSTR("v." FWVER_NAME));
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, u8g2.getDisplayHeight()-5, s);
}

void ViewLogo::process() {
    waitn ++;
    if (waitn < LOGO_TIME)
        return;
    
    setViewMain(cfg.d().dsplpwron);
}


static ViewLogo vLogo;
void setViewLogo() { viewSet(vLogo); }
