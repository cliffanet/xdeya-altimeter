
#include "../../def.h"

/* *********************************************************************
 *  Отображение основного режима с показаниями прибора
 *  Тут только отображение нескольких вариантов экрана с переключением
 *  между ними
 * *********************************************************************/

#define MODE_MAIN_MIN   1
#define MODE_MAIN_MAX   2
// Экран отображения только показаний GPS
#define MODE_MAIN_GPS   1
// Экран отображения времени
#define MODE_MAIN_TIME  2

static uint8_t mode = MODE_MAIN_GPS; // Текущая страница отображения, сохраняется при переходе в меню

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки компаса, провёрнутого на угол ang
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          round(cos(ang) * (x - cx) - sin(ang) * (y - cy)) + cx,  round(sin(ang) * (x - cx) + cos(ang) * (y - cy)) + cy
// APNT - упрощённая версия PNT, где ANG берётся автоматически из текущей зоны видимости кода
#define APNT(x,y,cx,cy)             PNT(x,y,ang,cx,cy)
inline void drawCompas(U8G2 &u8g2, float ang) {
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
inline void drawPointArrow(U8G2 &u8g2, float ang) {
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
//uint16_t testDeg();

static void displayGPS(U8G2 &u8g2) {
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
    //    drawCompas(u8g2, DEG_TO_RAD*(360 - testDeg()));
    //    drawPointArrow(u8g2, DEG_TO_RAD*(testDeg()));

    // Компас и стрелка к точке внутри него
    if (gps.course.isValid() && gps.location.isValid()) {
        // Компас показывает, куда смещено направление нашего движения 
        // относительно сторон Света,
        drawCompas(u8g2, DEG_TO_RAD*(360 - gps.course.deg()));
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
                drawPointArrow(u8g2, DEG_TO_RAD*(courseto-gps.course.deg()));
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
static void displayTime(U8G2 &u8g2) {
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
 *  Выбор отображения нужной страницы
 * ------------------------------------------------------------------------------------------- */
static void displayHnd() {  // назначение рисовальщика по экрану
    switch (mode) {
        case MODE_MAIN_GPS:     displayHnd(displayGPS); break;
        case MODE_MAIN_TIME:    displayHnd(displayTime); break;
    }
}

static void btnSelSmpl() {  // Обработка нажатия на среднюю кнопку
    mode ++;
    if (mode > MODE_MAIN_MAX) mode = MODE_MAIN_MIN;
    Serial.print(F("main next: "));
    Serial.println(mode);
    displayHnd();
}

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация главного режима отображения показаний
 * ------------------------------------------------------------------------------------------- */
void modeMain() {
    displayHnd();
    btnHndClear();
    btnHnd(BTN_SEL, BTN_SIMPLE, btnSelSmpl);
    btnHnd(BTN_SEL, BTN_LONG,   modeMenu);
    
    Serial.println(F("mode main"));
}
