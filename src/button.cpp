
#include "button.h"
#include <vector>

/* ------------------------------------------------------------------------------------------- *
 * Инициализация кнопок
 * ------------------------------------------------------------------------------------------- */
static volatile btn_t btnall[] = {
    { BUTTON_PIN_UP,    BTN_UP },
    { BUTTON_PIN_SEL,   BTN_SEL },
    { BUTTON_PIN_DOWN,  BTN_DOWN },
};

static volatile button_hnd_t hndlast = NULL;

static volatile uint8_t _state = 0;
static volatile uint32_t _statelast = 0;
// this variable will be changed in the ISR (interrupt), and Read in main loop
// static: says this variable is only visible to function in this file, its value will persist, it is a global variable
// volatile: tells the compiler that this variables value must not be stored in a CPU register, it must exist
//   in memory at all times.  This means that every time the value of intTriggeredCount must be read or
//   changed, it has be read from memory, updated, stored back into RAM, that way, when the ISR 
//   is triggered, the current value is in RAM.  Otherwise, the compiler will find ways to increase efficiency
//   of access.  One of these methods is to store it in a CPU register, but if it does that,(keeps the current
//   value in a register, when the interrupt triggers, the Interrupt access the 'old' value stored in RAM, 
//   changes it, then returns to whatever part of the program it interrupted.  Because the foreground task,
//   (the one that was interrupted) has no idea the RAM value has changed, it uses the value it 'know' is 
//   correct (the one in the register).  

/* ------------------------------------------------------------------------------------------- *
 * Проверка состояния
 * ------------------------------------------------------------------------------------------- */
void IRAM_ATTR btnChkState(volatile btn_t &b) {
    uint8_t val = digitalRead(b.pin);
    uint32_t m = millis();
    uint32_t tm = m-b.lastchg;
    
    if (tm >= BTN_FILTER_TIME) { // Защита от дребезга
        bool pushed = b.val == LOW; // Была ли нажата кнопка всё это время
        if (!pushed && (b.pushed != 0))  { // кнопка отпущена давно, можно сбросить флаги сработавших событий
            b.pushed = 0;
        }
        
        if (pushed) {
            if (((b.pushed & BTN_PUSHED_SIMPLE) == 0) && ((val != LOW) || (b.hndlong == NULL))) {
                b.pushed |= BTN_PUSHED_SIMPLE;
                hndlast = b.hndsmpl; // Простое нажатие
            }
            if (((b.pushed & BTN_PUSHED_LONG) == 0) && ((m-b.lastchg) >= BTN_LONG_TIME)) {
                b.pushed |= BTN_PUSHED_LONG | BTN_PUSHED_SIMPLE; // чтобы после длинного нажатия не сработало простое, помечаем простое тоже сработавшим
                hndlast = b.hndlong; // Длинное нажатие
            }
        }
    }
    
    if (b.val != val)
        b.lastchg = m; // время крайнего изменения состояния кнопки
    b.val = val;
    
    // текущее положение всех кнопок и их крайнее изменение
    uint8_t bmask = 1 << (b.code-1);
    uint8_t state = (_state & ~bmask) | ((val == LOW ? 1 : 0) << (b.code-1));
    if (_state != state) {
        _state = state;
        _statelast = m;
    }
}
void IRAM_ATTR btnChkState0() { btnChkState(btnall[0]); }
void IRAM_ATTR btnChkState1() { btnChkState(btnall[1]); }
void IRAM_ATTR btnChkState2() { btnChkState(btnall[2]); }

/* ------------------------------------------------------------------------------------------- *
 * Инициализация - включаем пины и прописываем прерывания
 * ------------------------------------------------------------------------------------------- */
void btnInit() {
    for (auto &b : btnall) {
        pinMode(b.pin, INPUT_PULLUP);
        if (b.pin == BUTTON_PIN_SEL) {
            // Если поисле включения питания мы всё ещё держим кнопку,
            // дальше пока не работаем
            while (digitalRead(BUTTON_GPIO_PWR) == LOW)
                delay(100);
        }
        b.val = digitalRead(b.pin);
    }
    
    attachInterrupt(
        digitalPinToInterrupt(btnall[0].pin),
        btnChkState0,
        CHANGE
    );
    attachInterrupt(
        digitalPinToInterrupt(btnall[1].pin),
        btnChkState1,
        CHANGE
    );
    attachInterrupt(
        digitalPinToInterrupt(btnall[2].pin),
        btnChkState2,
        CHANGE
    );
}

/* ------------------------------------------------------------------------------------------- *
 * Периодическая обработка
 * ------------------------------------------------------------------------------------------- */
void btnProcess() {
    for (auto &b : btnall)
        btnChkState(b);
    
    if (hndlast != NULL) {
        hndlast();
        hndlast = NULL;
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Очистка всех назначенных обработчиков нажатий - нежно при смене режима
 * ------------------------------------------------------------------------------------------- */
void btnHndClear() {
    for (auto &b : btnall) {
        b.hndsmpl = NULL;
        b.hndlong = NULL;
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Установка одного обработчика нажатия на кнопку
 * ------------------------------------------------------------------------------------------- */
void btnHnd(btn_code_t btn, button_time_t tm, button_hnd_t hnd) {
    volatile btn_t *b;
    switch (btn) {
        case BTN_UP:    b = &(btnall[0]); break;
        case BTN_SEL:   b = &(btnall[1]); break;
        case BTN_DOWN:  b = &(btnall[2]); break;
        default: return;
    }
    
    switch (tm) {
        case BTN_SIMPLE:    b->hndsmpl = hnd; break;
        case BTN_LONG:      b->hndlong = hnd; break;
    }
}


/* ------------------------------------------------------------------------------------------- *
 *  время ненажатия ни на одну кнопку
 * ------------------------------------------------------------------------------------------- */
uint32_t btnIdle() {
    if (_state != 0)
        return 0;
    return millis() - _statelast;
}

/* ------------------------------------------------------------------------------------------- *
 *  время зажатой одной или более кнопок
 * ------------------------------------------------------------------------------------------- */
uint32_t btnPressed(uint8_t &btn) {
    if (_state == 0)
        return 0;
    btn = _state;
    return millis() - _statelast;
}


/* ------------------------------------------------------------------------------------------- *
 *  отладочная функция, эмулирующая нажатие на кнопку
 * ------------------------------------------------------------------------------------------- */
void btnPush(btn_code_t btn, button_time_t tm) {
    _statelast = millis();
    
    volatile btn_t *b;
    switch (btn) {
        case BTN_UP:    b = &(btnall[0]); break;
        case BTN_SEL:   b = &(btnall[1]); break;
        case BTN_DOWN:  b = &(btnall[2]); break;
        default: return;
    }
    
    switch (tm) {
        case BTN_SIMPLE:    if (b->hndsmpl != NULL) (b->hndsmpl)(); break;
        case BTN_LONG:      if (b->hndlong != NULL) (b->hndlong)(); break;
    }
}
