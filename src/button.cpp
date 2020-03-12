
#include "button.h"
#include <vector>

/* ------------------------------------------------------------------------------------------- *
 * Инициализация кнопок
 * ------------------------------------------------------------------------------------------- */
static btn_t btnall[] = {
    { BUTTON_PIN_UP,    BTN_UP },
    { BUTTON_PIN_SEL,   BTN_SEL },
    { BUTTON_PIN_DOWN,  BTN_DOWN },
};

static std::vector<button_hnd_t> hndall;

static uint8_t _state = 0;
static uint32_t _statelast = 0;

/* ------------------------------------------------------------------------------------------- *
 * Проверка состояния
 * ------------------------------------------------------------------------------------------- */
void btnChkState(btn_t &b) {
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
                if (b.hndsmpl != NULL) hndall.push_back(b.hndsmpl); // Простое нажатие
            }
            if (((b.pushed & BTN_PUSHED_LONG) == 0) && ((m-b.lastchg) >= BTN_LONG_TIME)) {
                b.pushed |= BTN_PUSHED_LONG | BTN_PUSHED_SIMPLE; // чтобы после длинного нажатия не сработало простое, помечаем простое тоже сработавшим
                if (b.hndlong != NULL) hndall.push_back(b.hndlong); // Длинное нажатие
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

/* ------------------------------------------------------------------------------------------- *
 * Инициализация - включаем пины и прописываем прерывания
 * ------------------------------------------------------------------------------------------- */
void btnInit() {
    for (auto &b : btnall) {
        pinMode(b.pin, INPUT_PULLUP);
        b.val = digitalRead(b.pin);
    }
    
    attachInterrupt(
        digitalPinToInterrupt(btnall[0].pin),
        []() { btnChkState(btnall[0]); },
        CHANGE
    );
    attachInterrupt(
        digitalPinToInterrupt(btnall[1].pin),
        []() { btnChkState(btnall[1]); },
        CHANGE
    );
    attachInterrupt(
        digitalPinToInterrupt(btnall[2].pin),
        []() { btnChkState(btnall[2]); },
        CHANGE
    );
}

/* ------------------------------------------------------------------------------------------- *
 * Периодическая обработка
 * ------------------------------------------------------------------------------------------- */
void btnProcess() {
    for (auto &b : btnall)
        btnChkState(b);
    if (hndall.size() > 0) {
        for (auto hnd: hndall)
            hnd();
        hndall.clear();
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
    btn_t *b;
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
