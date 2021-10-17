
#include "logo.h"
#include "main.h"
#include "../cfg/main.h"

void ViewLogo::draw(U8G2 &u8g2) {
#if HWVER == 4
#define FONTNAME    u8g2_font_open_iconic_other_8x_t
#define FONTSIZE    56
#else
#define FONTNAME    u8g2_font_open_iconic_other_4x_t
#define FONTSIZE    28
#endif
    
    u8g2.setFont(FONTNAME);
    u8g2.drawGlyph((u8g2.getDisplayWidth()-FONTSIZE)/2, (u8g2.getDisplayHeight()-32)/2 + FONTSIZE/2, 0x40);
    
    char s[30];
    u8g2.setFont(u8g2_font_ImpactBits_tr);
    strcpy_P(s, PSTR("XDEYA"));
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, u8g2.getDisplayHeight()-20, s);
    strcpy_P(s, PSTR("v" FWVER_NAME));
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
