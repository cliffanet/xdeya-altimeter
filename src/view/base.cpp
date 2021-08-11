
#include "base.h"
#include "../cfg/main.h"
#include "../jump/proc.h"

/* ------------------------------------------------------------------------------------------- *
 *  Текущий View
 * ------------------------------------------------------------------------------------------- */
static ViewBase *vCur = NULL;

void viewSet(ViewBase &v) {
    vCur = &v;
}



/* ------------------------------------------------------------------------------------------- *
 *  Дисплей
 * ------------------------------------------------------------------------------------------- */

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

//U8G2_ST7565_ZOLEN_128X64_1_4W_HW_SPI u8g2(U8G2_MIRROR, /* cs=*/ 5, /* dc=*/ 33, /* reset=*/ 25);

#if HWVER >= 4
static U8G2_ST75256_JLX19296_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 26, /* dc=*/ 25, /* reset=*/ 33);
#else
static U8G2_UC1701_MINI12864_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 26, /* dc=*/ 33, /* reset=*/ 25);
#endif

void displayUpdate() {
    static auto vPrev = vCur;
    if (vPrev != vCur) {
        u8g2.clearDisplay();
        vPrev = vCur;
    }
    
    if (vCur == NULL)
        return;
     
    u8g2.firstPage();
    do {
        vCur->draw(u8g2);
    }  while( u8g2.nextPage() );
}

/* ------------------------------------------------------------------------------------------- *
 * Управление дисплеем: вкл/выкл самого дисплея и его подсветки, изменение контрастности
 * ------------------------------------------------------------------------------------------- */
static bool lght = false;
static void displayLightUpd();
void displayOn() {
    u8g2.setPowerSave(false);
    u8g2.clearDisplay();
    displayLightUpd();
}
void displayOff() {
    u8g2.setPowerSave(true);
#if HWVER == 1
    digitalWrite(LIGHT_PIN, LOW);
#else
    digitalWrite(LIGHT_PIN, HIGH);
#endif
}

static void displayLightUpd() {
#if HWVER == 1
    digitalWrite(LIGHT_PIN, lght ? HIGH : LOW);
#else
    digitalWrite(LIGHT_PIN, lght ? LOW : HIGH);
#endif
}

void displayLightTgl() {
    lght = not lght;
    displayLightUpd();
}

bool displayLight() {
    return lght;
}

void displayContrast(uint8_t val) {
#if HWVER >= 4
    u8g2.setContrast(115+val/2);
#else
    u8g2.setContrast(115+val*3);
#endif
}


/* ------------------------------------------------------------------------------------------- *
 *  Обработчик кнопок
 * ------------------------------------------------------------------------------------------- */
static volatile btn_t btnall[] = {
    { BUTTON_PIN_UP,    BTN_UP },
    { BUTTON_PIN_SEL,   BTN_SEL },
    { BUTTON_PIN_DOWN,  BTN_DOWN },
};
static volatile uint8_t _btnstate = 0;
static volatile uint32_t _btnstatelast = 0;
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

/* ------------------------------------------------------------------------------------------- */

static bool btnUseLong(btn_code_t btn) {
    if (vCur == NULL)
        return false;
    return vCur->useLong(btn);
}

/* ------------------------------------------------------------------------------------------- */

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
            if (((b.pushed & BTN_PUSHED_SIMPLE) == 0) && ((val != LOW) || !btnUseLong(b.code))) {
                b.pushed |= BTN_PUSHED_SIMPLE;
                b.evsmpl = true;
            }
            if (((b.pushed & BTN_PUSHED_LONG) == 0) && ((m-b.lastchg) >= BTN_LONG_TIME)) {
                b.pushed |= BTN_PUSHED_LONG | BTN_PUSHED_SIMPLE; // чтобы после длинного нажатия не сработало простое, помечаем простое тоже сработавшим
                b.evlong = true;
            }
        }
    }
    
    if (b.val != val)
        b.lastchg = m; // время крайнего изменения состояния кнопки
    b.val = val;
    
    // текущее положение всех кнопок и их крайнее изменение
    uint8_t bmask = 1 << (b.code-1);
    uint8_t state = (_btnstate & ~bmask) | ((val == LOW ? 1 : 0) << (b.code-1));
    if (_btnstate != state) {
        _btnstate = state;
        _btnstatelast = m;
    }
}
void IRAM_ATTR btnChkState0() { btnChkState(btnall[0]); }
void IRAM_ATTR btnChkState1() { btnChkState(btnall[1]); }
void IRAM_ATTR btnChkState2() { btnChkState(btnall[2]); }


/* ------------------------------------------------------------------------------------------- *
 *  время ненажатия ни на одну кнопку
 * ------------------------------------------------------------------------------------------- */
uint32_t btnIdle() { // используется только для выхода из меню
    if (_btnstate != 0)
        return 0;
    return millis() - _btnstatelast;
}

/* ------------------------------------------------------------------------------------------- *
 *  нажата ли кнопка
 * ------------------------------------------------------------------------------------------- */
bool btnPushed(btn_code_t btn) {
    for (const auto &b: btnall)
        if (b.code == btn)
            return b.val == LOW;
    return false;
}

/* ------------------------------------------------------------------------------------------- *
 *  время зажатой одной или более кнопок
 * ------------------------------------------------------------------------------------------- */
uint32_t btnPressed(uint8_t &btn) { // используется только для быстрого изменения значения зажатой кнопкой в меню
    if (_btnstate == 0)
        return 0;
    btn = _btnstate;
    return millis() - _btnstatelast;
}

#ifdef USE4BUTTON
/* ------------------------------------------------------------------------------------------- *
 *  нажатие на дополнительную 4ую кнопку
 * ------------------------------------------------------------------------------------------- */
bool btn4Pushed() {
    return digitalRead(BUTTON_PIN_4) == LOW;
}
#endif

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация view (кнопок и дисплея сразу)
 * ------------------------------------------------------------------------------------------- */
void viewInit() {
    // дисплей
    u8g2.begin();
    displayOn();
    u8g2.setFont(u8g2_font_helvB10_tr);  
    do {
        u8g2.drawStr(0,24,"Init...");
    } while( u8g2.nextPage() );
    
    pinMode(LIGHT_PIN, OUTPUT);
    
    // кнопки
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

#ifdef USE4BUTTON
    pinMode(BUTTON_PIN_4, INPUT_PULLUP);
#endif
}

/* ------------------------------------------------------------------------------------------- *
 * Периодическая обработка
 * ------------------------------------------------------------------------------------------- */
void viewProcess() {
    bool ballow =           // разрешены ли кнопки
        (vCur != NULL) &&   // указан текущий view
                            // не падаем ли мы (Запрет использования кнопок в принудительном FF-режиме)
        (!cfg.d().dsplautoff || (altCalc().state() != ACST_FREEFALL));
    
    for (auto &b : btnall) {
        btnChkState(b);
    
        if (b.evsmpl) {
            if (ballow)
                vCur->btnSmpl(b.code);
            b.evsmpl = false;
        }
        if (b.evlong) {
            if (ballow)
                vCur->btnLong(b.code);
            b.evlong = false;
        }
    }
    
    displayUpdate();
    
    if (vCur != NULL)
        vCur->process();
}
