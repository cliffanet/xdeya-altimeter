
#include "def.h"


static uint32_t btnHolded = 0;
static uint8_t btpPushed = 0;

/* ------------------------------------------------------------------------------------------- *
 * События кнопок: нажатие, отпускание, удержание
 * ------------------------------------------------------------------------------------------- */
static void btnPush(uint8_t btn) {
    Serial.print("btn push: ");
    Serial.println(btn);
    btnHolded = millis();
    
    if (!is_on)
        return;
    
    switch (displayMode()) {
        case DISP_MENU:
            if (btn == BTN_UP)
                menuNext();
            else
            if (btn == BTN_DOWN)
                menuPrev();
            break;
    }
}

static void btnRelease(uint8_t btn, uint8_t aft) {
    Serial.print("btn release: ");
    Serial.print(btn);
    Serial.print(", after: ");
    Serial.println(aft);
    // при отпускании любой кнопки выключаем таймер удержания кнопки
    btnHolded = 0;
    
    if (!is_on)
        return;
    
    switch (displayMode()) {
        case DISP_MAIN:
            displaySetMode(DISP_TIME);
            break;
        case DISP_TIME:
            displaySetMode(DISP_MAIN);
            break;
        case DISP_MENU:
            if ((btn == BTN_SEL) && (aft == 0))
                menuSelect();
            break;
    }
}

static bool btnHold(uint8_t btn, uint32_t bhld) {
    Serial.print("btn hold: ");
    Serial.print(btn);
    Serial.print(", time: ");
    Serial.println(bhld);
    if (!is_on) {
        if (btn != BTN_SEL) return true;
        if (bhld < BTNTIME_PWRON) return false;
        pwrOn();
        return true;
    }
    switch (displayMode()) {
        case DISP_MENU:
            if (btn == BTN_SEL) {
                if (bhld < BTNTIME_MENUEXIT)
                    return false;
                menuExit();
                return true;
            }
            break;
            
        case DISP_MENUHOLD:
            return menuHold(btn, bhld);
            
        case DISP_MAIN:
            if (btn == BTN_SEL) {
                if (bhld < BTNTIME_MENUENTER)
                    return false;
                menuShow();
                return true;
            }
            break;
    }

    return true;
}

/* ------------------------------------------------------------------------------------------- *
 * Функция слежения за состояниями кнопок
 * ------------------------------------------------------------------------------------------- */
uint8_t btnRead() {
    uint8_t prev = btpPushed;

    btpPushed = 0;

    if (digitalRead(BUTTON_PIN_UP) == LOW)
        btpPushed |= BTN_UP;
    if (digitalRead(BUTTON_PIN_SEL) == LOW)
        btpPushed |= BTN_SEL;
    if (digitalRead(BUTTON_PIN_DOWN) == LOW)
        btpPushed |= BTN_DOWN;

    // все кнопки как были отжаты, так и остаются
    if ((btpPushed == prev) && (btnHolded == 0))
        return btpPushed;

    if (btpPushed > prev) {
        // была  нажата какая-то кнопка, которая не была нажата до этого, запускаем таймер
        btnPush(btpPushed);
    }
    else
    if ((btpPushed == prev) && (btnHolded > 0)) {
        // продолжаем удерживать кнопку
        if (btnHold(btpPushed, millis()-btnHolded))
            btnHolded = 0;
    }
    else 
    if ((btpPushed < prev) && (btnHolded > 0)) {
        // какая-то из кнопок была отпущена
        btnRelease(prev, btpPushed);
    }

    return btpPushed;
}

/* ------------------------------------------------------------------------------------------- *
 * Инициализация прерываний для кнопок
 * ------------------------------------------------------------------------------------------- */
void btnInterrupt() {
#define DOINT(_btnn) \
    attachInterrupt( \
        digitalPinToInterrupt(BUTTON_PIN_ ## _btnn), \
        []() { \
            uint8_t prev = btpPushed; \
            if (digitalRead(BUTTON_PIN_ ## _btnn) == LOW) { \
                btpPushed |= BTN_ ## _btnn; \
                if (prev != btpPushed) btnPush(btpPushed); \
            } \
            else { \
                btpPushed &= ~BTN_ ## _btnn; \
                if ((prev != btpPushed) && (btnHolded > 0)) \
                    btnRelease(prev, btpPushed); \
            } \
        }, \
        CHANGE \
    );
    
    DOINT(UP);
    DOINT(SEL);
    DOINT(DOWN);
}
