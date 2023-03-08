/*
    View: Base
*/

#ifndef _view_base_H
#define _view_base_H

#include "../../def.h"
#include <U8g2lib.h>
#include "text.h"

/* ------------------------------------------------------------------------------------------- *
 *  Дисплей
 * ------------------------------------------------------------------------------------------- */

// Пин подсветки дисплея
#define LIGHT_PIN 32

// принудительная перерисовка экрана
void displayUpdate();
void displayDraw(void (*draw)(U8G2 &u8g2), bool clear = true, bool init = false);

/* Управление дисплеем */
void displayOn();
void displayOff();
void displayLightTgl();
bool displayLight();
void displayContrast(uint8_t value);
void displayFlipp180(bool flipp);


/* ------------------------------------------------------------------------------------------- *
 *  Кнопки
 * ------------------------------------------------------------------------------------------- */

// Пины кнопок
//#define BUTTON_PIN_UP     14
//#define BUTTON_PIN_SEL    27
//#define BUTTON_PIN_DOWN   26
#if HWVER <= 1

#define BUTTON_PIN_UP       12
#define BUTTON_PIN_SEL      14
#define BUTTON_PIN_DOWN     27

#ifdef USE4BUTTON
#define BUTTON_PIN_4        13
#endif

#define BUTTON_GPIO_PWR   GPIO_NUM_14

#else

#define BUTTON_PIN_UP       39
#define BUTTON_PIN_SEL      34
#define BUTTON_PIN_DOWN     35

#define BUTTON_GPIO_PWR   GPIO_NUM_34

#ifdef USE4BUTTON
#define BUTTON_PIN_4        12
#endif

#endif

// Коды кнопок
typedef enum {
    BTN_UP = 1,
    BTN_DOWN,
    BTN_SEL
} btn_code_t;

typedef struct {
    uint8_t     pin;
    btn_code_t  code;       // Код кнопки
    uint8_t     val;        // текущее состояние
    uint8_t     pushed;     // сработало ли уже событие pushed, чтобы повторно его не выполнить, пока кнопка не отпущена
    bool        evsmpl;
    bool        evlong;
    uint32_t    lastchg;    // millis() крайнего изменения состояния
} btn_t;

// минимальное время между изменениями состояния кнопки
#define BTN_FILTER_TIME     50
// время длинного нажатия
#define BTN_LONG_TIME       2000

// Флаги сработавших событий нажатия
#define BTN_PUSHED_SIMPLE   0x01
#define BTN_PUSHED_LONG     0x02

// отдельно выносим инициализацию кнопок,
// т.к. оно нужно при возвращении из deepsleep
void btnInit();

// время ненажатия ни на одну кнопку
uint32_t btnIdle();
// получение маски из кода кнопки, используется в btnPressed
#define btnMask(bcode)      (1 << (bcode-1))
// время зажатой одной или более кнопок
uint32_t btnPressed(uint8_t &btn);
// нажата ли сейчас кнопка
bool btnPushed(btn_code_t btn);
// Ожидание, пока кнопка будет отпущена
void btnWaitRelease();
// Переворот на 180
void btnFlipp180(bool flipp);

#ifdef USE4BUTTON
// нажатие на дополнительную 4ую кнопку
bool btn4Pushed();
#endif

/* ------------------------------------------------------------------------------------------- *
 *  View
 * ------------------------------------------------------------------------------------------- */

typedef struct { int x, y; } pnt_t;

class View {
    public:
        virtual void btnSmpl(btn_code_t btn) { }
        virtual void btnLong(btn_code_t btn) { }
        virtual bool useLong(btn_code_t btn) { return false; }
        // признак того, что надо оставаться в pwrActive
        virtual bool isActive() { return false; }
        
        virtual void draw(U8G2 &u8g2) = 0;
        
        virtual void process() {}
        
        static bool isblink();
};

void viewSet(View &v);
bool viewIs(View &v);
bool viewActive();

void viewInit();
void viewProcess();

#endif // _view_base_H
