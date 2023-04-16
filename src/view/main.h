/*
    View: Main
*/

#ifndef _view_main_H
#define _view_main_H

#include "base.h"

// Вырианты отображения основного режима
#define MODE_MAIN_MIN       1
#define MODE_MAIN_MAX       3
// не переключать на main-экран (ничего не делать)
#define MODE_MAIN_NONE      -1
// Использовать крайний запомненный (не менять экран, но включить main-режим)
#define MODE_MAIN_LAST      0
// Экран отображения высоты и NAVI
#define MODE_MAIN_ALTNAV    1
// Экран отображения высоты (большими цифрами)
#define MODE_MAIN_ALT       2
// Экран отображения навигации
#define MODE_MAIN_NAV       3
// Экран отображения пройденного пути
#define MODE_MAIN_NAVPATH   4

class ViewMain : public View {
    public:
        void btnLong(btn_code_t btn);
        bool useLong(btn_code_t btn) { return true; };
        
        void drawState(U8G2 &u8g2);
        void drawClock(U8G2 &u8g2);
};

void setViewMain(int8_t mode = MODE_MAIN_LAST, bool save = true);

//void setViewMainGps();
void setViewMainAltNav();
void setViewMainAlt();
void setViewMainNav();
void setViewMainNavPath();

void navPathAdd(int mode, bool valid, int32_t lon, int32_t lat);

#endif // _view_main_H
