
#include "def.h"
#include <Arduino.h>


#include <TinyGPS++.h>
TinyGPSPlus gps;
TinyGPSPlus &gpsGet() { return gps; }
// будем использовать стандартный экземпляр класса HardwareSerial, 
// т.к. он и так в системе уже есть и память под него выделена
// Стандартные пины для свободного аппаратного Serial2: 16(rx), 17(tx)
#define ss Serial2


#include <TimeLib.h>
static uint32_t tmadj = 0;
bool timeOk() { return (tmadj > 0) && ((tmadj > millis()) || ((millis()-tmadj) >= TIME_ADJUST_TIMEOUT)); }

bool is_on = true;

//------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    
    displayInit();
    
    // инициируем uart-порт GPS-приёмника
    ss.begin(9600);

    // инициируем кнопки
    btnInit();

    pinMode(LIGHT_PIN, OUTPUT);

    // загружаем сохранённый конфиг
    cfgLoad();
    cfgApply();
    Serial.println("begin");
    Serial.println(sizeof(cfg));

    modeMain();
}

//------------------------------------------------------------------------------
void pwrOn() {
    if (is_on) return;
    is_on = true;
    displaySetMode(DISP_MAIN);
    displayOn();
        Serial.println("pwr on");
}

void pwrOff() {
    if (!is_on) return;
    is_on = false;
    displayOff();
        Serial.println("pwr off");
}

//------------------------------------------------------------------------------



uint16_t _testDeg = 0;
uint16_t testDeg() { return _testDeg; }

//------------------------------------------------------------------------------
void loop() {
    // читаем состояние кнопок
    //uint8_t btn = btnRead();
    
    if (!is_on) {
        delay(1000); // Использование прерываний позволяет делать более длинный паузы в состоянии Off
        return;
    }

    while (ss.available()) {
        gps.encode( ss.read() );
    }

    if (millis() >= tmadj) {
        if (gps.time.age() < 500) {
            // set the Time to the latest GPS reading
            setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
            adjustTime(cfg.timezone * 60);
            tmadj = millis() + TIME_ADJUST_INTERVAL;
        }
    }

    static bool disp_init = true;
    if (disp_init && (displayMode() == DISP_INIT) && gps.satellites.isValid()) {
        displaySetMode(DISP_MAIN);
        disp_init = false;
    }

    switch (displayMode()) {
        case DISP_MAIN:
            _testDeg += 5;
            displayChange(DISP_MAIN, 500);
            break;
            
        case DISP_TIME:
            displayChange(DISP_TIME, 1000);
            break;
            
        case DISP_MENU:
            menuTimeOut();
            break;
            
        case DISP_MENUHOLD:
            //if (btn == 0) menuHold(0, 0);
            break;
    }
    
    btnProcess();
    displayUpdate();
    
    delay(100);
    btnProcess();
    delay(100);
}
