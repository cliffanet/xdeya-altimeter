
#include "def.h"

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

//U8G2_ST7565_ZOLEN_128X64_1_4W_HW_SPI u8g2(U8G2_MIRROR, /* cs=*/ 5, /* dc=*/ 33, /* reset=*/ 25);

U8G2_UC1701_MINI12864_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 26, /* dc=*/ 33, /* reset=*/ 25);

static display_mode_t mode_curr = DISP_INIT, mode_prev = DISP_INIT;
static uint8_t change = 0;
static uint32_t updated = 0;


/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки компаса, провёрнутого на угол ang
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          round(cos(ang) * (x - cx) - sin(ang) * (y - cy)) + cx,  round(sin(ang) * (x - cx) + cos(ang) * (y - cy)) + cy
// APNT - упрощённая версия PNT, где ANG берётся автоматически из текущей зоны видимости кода
#define APNT(x,y,cx,cy)             PNT(x,y,ang,cx,cy)
inline void drawCompas(float ang) {
    u8g2.setFont(u8g2_font_helvB08_tr);
    
    u8g2.drawDisc(32, 31, 1);
    //u8g2.drawCircle(32,31, 31);
    u8g2.drawDisc(APNT(31,6,31,32), 7);
    u8g2.setDrawColor(0);
    u8g2.drawGlyph(APNT(28,10,28,36), 'N');
    u8g2.setDrawColor(1);
    u8g2.drawGlyph(APNT(28,64,28,36), 'S');
    u8g2.drawGlyph(APNT(1,36,28,36), 'W');
    u8g2.drawGlyph(APNT(55,36,28,36), 'E');
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки стрелки направления к точке внутри компаса
 * ------------------------------------------------------------------------------------------- */
inline void drawPointArrow(float ang) {
    //u8g2.drawLine(APNT(32,15,32,31), APNT(32,42,32,31));
    u8g2.drawLine(APNT(32,15,32,31), APNT(30,36,32,31));
    u8g2.drawLine(APNT(32,15,32,31), APNT(34,36,32,31));
    u8g2.drawLine(APNT(30,36,32,31), APNT(34,36,32,31));
    u8g2.drawLine(APNT(32,15,32,31), APNT(28,20,32,31));
    u8g2.drawLine(APNT(32,15,32,31), APNT(36,20,32,31));
    u8g2.drawLine(APNT(32,14,32,31), APNT(27,20,32,31));
    u8g2.drawLine(APNT(32,14,32,31), APNT(37,20,32,31));
    u8g2.drawLine(APNT(32,16,32,31), APNT(28,21,32,31));
    u8g2.drawLine(APNT(32,16,32,31), APNT(36,21,32,31));
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки всей информации о GPS
 * ------------------------------------------------------------------------------------------- */
uint16_t testDeg();

inline void displayMain() {
    TinyGPSPlus &gps = gpsGet();
    char s[50];
    
    u8g2.setFont(u8g2_font_ncenB08_tr); 
    
    // количество спутников в правом верхнем углу
    if (gps.satellites.value() > 0)
        sprintf_P(s, PSTR("sat: %d"), gps.satellites.value());
    else
        strcpy_P(s, PSTR("no sat :("));
    u8g2.drawStr(65, 8, s);

    if (gps.satellites.value() == 0)
        return;

    // Текущие координаты
    if (gps.location.isValid()) {
        sprintf_P(s, PSTR("la:%f"), gps.location.lat());
        u8g2.drawStr(65, 22, s);
        sprintf_P(s, PSTR("lo:%f"), gps.location.lng());
        u8g2.drawStr(65, 34, s);
    }

    bool in_pnt = false;
    if (gps.location.isValid() && PNT_NUMOK && POINT.used) {
        double dist = 
            TinyGPSPlus::distanceBetween(
                gps.location.lat(),
                gps.location.lng(),
                POINT.lat, 
                POINT.lng
            );

        in_pnt = dist < 8.0;
        
        u8g2.setFont(u8g2_font_ncenB18_tr); 
        if (dist < 950) 
            sprintf_P(s, PSTR("%dm"), (int)round(dist));
        else if (dist < 9500) 
            sprintf_P(s, PSTR("%0.1fkm"), dist/1000);
        else if (dist < 950000) 
            sprintf_P(s, PSTR("%dkm"), (int)round(dist/1000));
        else
            sprintf_P(s, PSTR("%0.2fMm"), dist/1000000);
        u8g2.drawStr(62, 64, s);
    }
    /*
    static bool in_pnt_prev = false;
    if (!in_pnt_prev && in_pnt)
        msgFlash(flashPntReached);
    in_pnt_prev = in_pnt;
    */
    //    drawCompas(DEG_TO_RAD*(360 - testDeg()));
    //    drawPointArrow(DEG_TO_RAD*(testDeg()));

    // Компас и стрелка к точке внутри него
    if (gps.course.isValid() && gps.location.isValid()) {
        // Компас показывает, куда смещено направление нашего движения 
        // относительно сторон Света,
        drawCompas(DEG_TO_RAD*(360 - gps.course.deg()));
        // а стрелка показывает отклонение направления к точке относительно 
        // направления нашего движения
        if (PNT_NUMOK && POINT.used) {
            if (in_pnt) {
                u8g2.drawCircle(32, 31, 10);
                u8g2.drawCircle(32, 31, 15);
            }
            else {
                double courseto = 
                    TinyGPSPlus::courseTo(
                        gps.location.lat(),
                        gps.location.lng(),
                        POINT.lat, 
                        POINT.lng
                    );
                drawPointArrow(DEG_TO_RAD*(courseto-gps.course.deg()));
            }
            
            u8g2.drawDisc(32, 31, 6);
            u8g2.setFont(u8g2_font_helvB08_tr);
            u8g2.setDrawColor(0);
            u8g2.drawGlyph(30,36, '0' + cfg.pntnum);
            u8g2.setDrawColor(1);
        }
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Текущего времени
 * ------------------------------------------------------------------------------------------- */
inline void displayTime() {
    char s[20];
    if (!timeOk()) {
        u8g2.setFont(u8g2_font_ncenB14_tr); 
        strcpy_P(s, PSTR("Clock wait"));
        u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 20);
        u8g2.print(s);
        strcpy_P(s, PSTR("GPS Sync..."));
        u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 50);
        u8g2.print(s);
        return;
    }

    u8g2.setFont(u8g2_font_logisoso38_tn); 
    sprintf_P(s, PSTR("%2d:%02d"), hour(), minute());
    u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 47);
    u8g2.print(s);

    u8g2.setFont(u8g2_font_ncenB08_tr); 
    sprintf_P(s, PSTR("%2d.%02d.%d"), day(), month(), year());
    u8g2.drawStr(0, 64, s);
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Меню
 * ------------------------------------------------------------------------------------------- */
inline void displayMenu() {
    menu_item_t ml[3];

    menuList(ml);
    u8g2.setFont(u8g2_font_helvB10_tr); 
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t pos = 4+(40*i);
        u8g2.setFont(ml[i].font);
        u8g2.drawGlyph(pos+3, 36, ml[i].icon);
    }

    u8g2.drawFrame(46, 3, 34, 34);
    u8g2.drawFrame(45, 2, 36, 36);
    u8g2.drawFrame(44, 1, 38, 38);
    
    u8g2.setFont(u8g2_font_helvB10_tr);
    char name[30], val[15];
    menuInfo(name, val);
    if (val[0] == '\0')
        u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(name))/2, u8g2.getDisplayHeight()-5);
    else
        u8g2.setCursor(0, u8g2.getDisplayHeight()-5);
    u8g2.print(name);
    if (val[0] != '\0') {
        u8g2.setCursor(u8g2.getDisplayWidth()-u8g2.getStrWidth(val)-4, u8g2.getDisplayHeight()-5);
        u8g2.print(val);
    }
}

inline void displayMenuHold() {
    menu_item_t ml[3];

    menuList(ml);
    u8g2.setFont(ml[1].font);
    u8g2.drawGlyph(47, 36, ml[1].icon);
    
    u8g2.setFont(u8g2_font_helvB10_tr);
    char name[30], val[15];
    menuInfo(name, val);
    u8g2.setCursor(0, u8g2.getDisplayHeight()-5);
    u8g2.print(name);
    u8g2.setCursor(u8g2.getDisplayWidth()-u8g2.getStrWidth(val)-4, u8g2.getDisplayHeight()-5);
    u8g2.print(val);
}

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции обновления дисплея
 * ------------------------------------------------------------------------------------------- */
void displayInit() {
    u8g2.begin();
    displayOn();
    u8g2.setFont(u8g2_font_helvB10_tr);  
    do {
        u8g2.drawStr(0,24,"Init...");
    } while( u8g2.nextPage() );
}

void displayUpdate() {
    if (change == 0)
        return;
    
    if (change > 1)
        u8g2.clearDisplay();
     
    u8g2.firstPage();
    do {
        switch (mode_curr) {
            case DISP_MAIN:
                displayMain();
                break;
            case DISP_TIME:
                displayTime();
                break;
            case DISP_MENU:
                displayMenu();
                break;
            case DISP_MENUHOLD:
                displayMenuHold();
                break;
        }
    } while( u8g2.nextPage() );

    updated = millis();
    change = 0;

    char s[29];
    sprintf_P(s, PSTR("%2d:%02d:%02d"), hour(), minute(), second());
    Serial.println(s);
}

static bool lght = false;
void displayOn() {
    u8g2.setPowerSave(false);
    u8g2.clearDisplay();
    change = 2;
    digitalWrite(LIGHT_PIN, lght ? HIGH : LOW);
}
void displayOff() {
    u8g2.setPowerSave(true);
    digitalWrite(LIGHT_PIN, LOW);
}

void displayLightTgl() {
    lght = not lght;
    digitalWrite(LIGHT_PIN, lght ? HIGH : LOW);
}

bool displayLight() {
    return lght;
}

void displayContrast(uint8_t val) {
    //u8g2.setContrast(val*3);
    u8g2.setContrast(115+val*3);
}

void displaySetMode(display_mode_t mode) {
    if (mode == mode_curr)
        return;
    mode_curr = mode;
    change = 2;
}

display_mode_t displayMode() {
    return mode_curr;
}

void displayChange(display_mode_t mode, uint32_t timeout) {
    if ((mode != mode_curr) || (change != 0))
        return;
    if ((updated > 0) && ((updated+timeout) > millis()))
        return;
    
    change = 1;
}
