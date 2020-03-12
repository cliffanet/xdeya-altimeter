/*
    Menu Base class
*/

#ifndef _menu_static_H
#define _menu_static_H

#include "base.h"
#include "../mode.h"

typedef void (*menu_hnd_t)();
typedef void (*menu_val_t)(char *txt);

// Элемент меню
typedef struct {
    char        *name;      // Текстовое название пункта
    menu_hnd_t  enter;      // Обраотка нажатия на среднюю кнопку
    menu_val_t  showval;    // как отображать значение, если требуется
    menu_hnd_t  editup;     // в режиме редактирования меняет значения вверх (функция должна описывать, что и как менять)
    menu_hnd_t  editdown;   // в режиме редактирования меняет значения вниз
    menu_hnd_t  hold;       // Обработка действия при длинном нажатии на среднюю кнопку
} menu_el_t;


class MenuStatic : public MenuBase {
    public:
        MenuStatic();
        MenuStatic(const menu_el_t *m, int16_t sz, const char *_title = NULL) : MenuBase(sz, _title), menu(m) {};
        void updStr(menu_dspl_el_t &str, int16_t i);
        
        void updHnd(int16_t i, button_hnd_t &smp, button_hnd_t &lng, button_hnd_t &editup, button_hnd_t &editdn);
    
    private:
        const menu_el_t *menu;
};

#endif // _menu_static_H
