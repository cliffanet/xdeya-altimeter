/*
    Button
*/

#ifndef _button_H
#define _button_H

#include <Arduino.h>

// Пины кнопок
//#define BUTTON_PIN_UP     14
//#define BUTTON_PIN_SEL    27
//#define BUTTON_PIN_DOWN   26
#define BUTTON_PIN_UP     12
#define BUTTON_PIN_SEL    14
#define BUTTON_PIN_DOWN   27

// Коды кнопок
#define BTN_UP    0x1
#define BTN_DOWN  0x2
#define BTN_SEL   0x8

typedef void (*button_hnd_t)();

typedef struct {
    uint8_t     pin;
    uint8_t     val;        // текущее состояние
    uint8_t     pushed;     // сработало ли уже событие pushed, чтобы повторно его не выполнить, пока кнопка не отпущена
    uint32_t    lastchg;    // millis() крайнего изменения состояния
    button_hnd_t hndsmpl;   // обработчик простого нажатия
    button_hnd_t hndlong;   // обработчик длинного нажатия
} btn_t;

// минимальное время между изменениями состояния кнопки
#define BTN_FILTER_TIME     50
// время длинного нажатия
#define BTN_LONG_TIME       2000

// Флаги сработавших событий нажатия
#define BTN_PUSHED_SIMPLE   0x01
#define BTN_PUSHED_LONG     0x02

typedef enum {
    BTN_SIMPLE,
    BTN_LONG
} button_time_t;

void btnInit();
void btnProcess();
void btnHndClear();
void btnHnd(uint8_t btn, button_time_t tm, button_hnd_t hnd);

#endif // _button_H
