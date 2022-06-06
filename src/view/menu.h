/*
    View: Menu
*/

#ifndef _view_menu_H
#define _view_menu_H

#include "base.h"

#if HWVER < 4
#define MENU_STR_COUNT  5
#else
#define MENU_STR_COUNT  8
#endif

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#define MENUSZ_NAME 40
#define MENUSZ_VAL 30
#else
#define MENUSZ_NAME 20
#define MENUSZ_VAL 15
#endif

#define MENU_TIMEOUT    15000

typedef struct {
    char name[MENUSZ_NAME] = { '\0' };
    char val[MENUSZ_VAL] = { '\0' };
} menu_dspl_el_t;

typedef enum {
    MENUEXIT_NONE,
    MENUEXIT_TOP,
    MENUEXIT_BOTTOM,
} menu_exit_t;

class ViewMenu : public ViewBase {
    public:
        ViewMenu(uint16_t _sz, menu_exit_t _exit = MENUEXIT_TOP);
        ViewMenu(menu_exit_t _exit = MENUEXIT_TOP) : ViewMenu(0, _exit) {};
        
        void setSize(uint16_t _sz);
        virtual
        void open(ViewMenu *_mprev = NULL, const char *_title = NULL);
        virtual
        void restore();
        
        // детектор, является ли указанный пункт меню "выходом"
        bool isExit(int16_t i) { return ((elexit == MENUEXIT_TOP) && (i == 0)) || ((elexit == MENUEXIT_BOTTOM) && (i+1 >= sz)); }

        virtual     // этот метод обновляет строку в потомке
        void getStr(menu_dspl_el_t &str, int16_t i) { };
        virtual     // указывает, надо ли моргать текущему значению (режим редактирования)
        bool valFlash() { return false; }
                    // все остальные - это обвязка по обновлению строк меню на экране в базовом классе
        void updStr();
        void updStr(int16_t i);
        void updStrSel() { updStr(isel); updValFlash(); }
        void setTop(int16_t i);  // установка индекса самого верхнего элемента (когда переместились курсором за пределы экрана)
        void setSel(int16_t i); // установка выбранного элемента
        void updValFlash();
        
        void btnSmpl(btn_code_t btn);
        
        void draw(U8G2 &u8g2);
        
        int16_t sel() const { return elexit == MENUEXIT_TOP ? isel-1 : isel; }
        int16_t size() const { return (elexit == MENUEXIT_NONE) || (sz <= 0) ? sz : sz-1; }
    
    protected:
        int16_t itop = 0, isel = 0, sz = 0;
        menu_exit_t elexit;
        ViewMenu *mprev = NULL;
        const char *titlep = NULL;
};
        
// Запоминаем текст сообщения и сколько тактов отображения показывать
void menuFlashP(const char *txt, int16_t count = 20);
// Стандартное сообщение о том, что для данной процедуры нужно удерживать среднюю кнопку три секунды
void menuFlashHold();
void menuFlashClear();

void setViewMenu();

bool menuIsWifi();
bool menuIsFwSd();

#endif // _view_menu_H
