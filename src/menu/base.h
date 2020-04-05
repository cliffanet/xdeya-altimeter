/*
    Menu Base class
*/

#ifndef _menu_base_H
#define _menu_base_H

#include <Arduino.h>

#define MENU_STR_COUNT  5
#define MENUSZ_NAME 20
#define MENUSZ_VAL 15

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

class U8G2;

class MenuBase {
    public:
        MenuBase(uint16_t _sz, const char *_title = NULL, menu_exit_t _exit = MENUEXIT_TOP);
        virtual             // виртуальный деструктор в базовом классе нужен, 
        ~MenuBase() { };    // чтобы корректно уничтожались потомки при выполнении delete
        
        virtual     // этот метод обновляет строку в потомке
        void updStr(menu_dspl_el_t &str, int16_t i) { Serial.println("MenuBase::updStr"); };
                    // все остальные - это обвязка по обновлению строк меню на экране в базовом классе
        void updStr();
        void updStr(int16_t i);
        void updStrSel() { updStr(isel); }
        
        void updHnd(); // обновляет обработчики текущего выделенного пункта меню
        
        void displayDraw(U8G2 &u8g2); // перерисовывает на экране всю видимую часть меню
        void setTop(int16_t i);  // установка индекса самого верхнего элемента (когда переместились курсором за пределы экрана)
        void selUp(); // движение курсора вверх/вниз по пунктам меню
        void selDn();
        
        void levelUp();     // выход в вышестоящее меню
        void exit();        // полный выход из меню
        
        // детектор, является ли указанный пункт меню "выходом"
        bool isExit(int16_t i) { return ((elexit == MENUEXIT_TOP) && (i == 0)) || ((elexit == MENUEXIT_BOTTOM) && (i+1 >= sz)); }
        const char *uptitle() { return _uptitle; }
        
        // Управление кнопками
        virtual void btnSmp() {} // Нажатие на среднюю кнопку короткое
        virtual bool useLng() { return false; } // используется ли длинное нажатие
        virtual void btnLng() {} // Нажатие на среднюю кнопку длинное
        virtual bool useEdit() { return false; } // используется ли редактирование
        virtual void editEnter() {} // вход в режим редактирования
        virtual void edit(int val) {} // изменение пункта меню на величину val:
                                                        // +-1 при коротком,
                                                        // +-10 при удержании 3 сек,
                                                        // +- 100 при удержании 6 сек
        
        int16_t sel() const { return elexit == MENUEXIT_TOP ? isel-1 : isel; }
        int16_t size() const { return (elexit == MENUEXIT_NONE) || (sz <= 0) ? sz : sz-1; }
        
    private:
        int16_t itop = 0, isel = 0, sz = 0;
        menu_exit_t elexit;
        char _uptitle[MENUSZ_NAME] = { '\0' };
};

void menuEnter(MenuBase *menu);
void menuClear(); // Только завершение режима меню - полный выход из всех меню
void modeMenu();
        
// Запоминаем текст сообщения и сколько тактов отображения показывать
void menuFlashP(char *txt, int16_t count = 20);
// Стандартное сообщение о том, что для данной процедуры нужно удерживать среднюю кнопку три секунды
void menuFlashHold();
void menuFlashClear();

#endif // _menu_base_H
