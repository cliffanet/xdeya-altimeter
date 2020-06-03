/*
    Menu Base class
*/

#ifndef _menu_static_H
#define _menu_static_H

#include "base.h"
#include "../mode.h"

typedef void (*menu_hnd_t)();
typedef void (*menu_edit_t)(int val);
typedef void (*menu_val_t)(char *txt);

// Элемент меню
typedef struct {
    char        *name;      // Текстовое название пункта
    menu_hnd_t  enter;      // Обраотка нажатия на среднюю кнопку
    menu_val_t  showval;    // как отображать значение, если требуется
    menu_edit_t edit;       // в режиме редактирования меняет значения на величину val (функция должна описывать, что и как менять)
    menu_hnd_t  hold;       // Обработка действия при длинном нажатии на среднюю кнопку
} menu_el_t;


class MenuStatic : public MenuBase {
    public:
        MenuStatic();
        MenuStatic(const menu_el_t *m, int16_t sz, const char *_title = NULL) : MenuBase(sz, _title), menu(m) {};
        void getStr(menu_dspl_el_t &str, int16_t i);
        
        void btnSmp();
        bool useLng();
        void btnLng();
        bool useEdit();
        void editEnter() { btnSmp(); }
        void edit(int val);
    
    private:
        const menu_el_t *menu;
};

#endif // _menu_static_H
