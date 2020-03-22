/*
    Menu Base class
*/

#ifndef _menu_logbook_H
#define _menu_logbook_H

#include "base.h"


class MenuLogBook : public MenuBase {
    public:
        MenuLogBook();
        void updStr(menu_dspl_el_t &str, int16_t i);
        
        void btnSmp();
};

#endif // _menu_logbook_H
