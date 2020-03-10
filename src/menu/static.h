/*
    Menu Base class
*/

#ifndef _menu_static_H
#define _menu_static_H

#include "base.h"
#include "../mode.h"

class MenuStatic : public MenuBase {
    public:
        MenuStatic();
        MenuStatic(const char *_title, const menu_el_t *m, int16_t sz) : MenuBase(_title, sz), menu(m) {};
        void updStr(menu_dspl_el_t &str, int16_t i);
        
        void updHnd(int16_t i, button_hnd_t &smp, button_hnd_t &lng, button_hnd_t &editup, button_hnd_t &editdn);
    
    private:
        const menu_el_t *menu;
};

#endif // _menu_static_H
