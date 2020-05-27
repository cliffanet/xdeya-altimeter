
#include "../mode.h"
#include "../display.h"
#include "../button.h"
#include "../cfg/point.h"
#include "../gps.h"
#include "../altimeter.h"
#include "../timer.h"
#include "../menu/static.h"
#include "../file/track.h"
#include "../../def.h" //time

/* *********************************************************************
 *  Отображение основного режима с показаниями прибора
 *  Тут только отображение нескольких вариантов экрана с переключением
 *  между ними
 * *********************************************************************/

bool modemain = true;

static uint8_t mode = MODE_MAIN_GPS; // Текущая страница отображения, сохраняется при переходе в меню


/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки состояния записи трека
 * ------------------------------------------------------------------------------------------- */
void drawTrackState(U8G2 &u8g2) {
    if (!trkRunning() || ((millis() % 2000) < 1000))
        return;
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setDrawColor(1);
    u8g2.drawGlyph(0, 10, 'R');
}

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

static void displayCompasFull(U8G2 &u8g2) {
    auto &gps = gpsGet();

    if (!gps.satellites.isValid() || (gps.satellites.value() == 0)) {
        char s[10];
        u8g2.setFont(u8g2_font_osb26_tr);
        strcpy_P(s, PSTR("NO"));
        u8g2.drawStr((62-u8g2.getStrWidth(s))/2, 28, s);
        strcpy_P(s, PSTR("SAT"));
        u8g2.drawStr((62-u8g2.getStrWidth(s))/2, 60, s);
        return;
    }
    
    // Компас и стрелка к точке внутри него
    if (gps.course.isValid() && gps.location.isValid()) {
        // Компас показывает, куда смещено направление нашего движения 
        // относительно сторон Света,
        drawCompas(u8g2, DEG_TO_RAD*(360 - gps.course.deg()));
        // а стрелка показывает отклонение направления к точке относительно 
        // направления нашего движения
        if (pnt.numValid() && pnt.cur().used) {
            bool in_pnt = 
                TinyGPSPlus::distanceBetween(
                    gps.location.lat(),
                    gps.location.lng(),
                    pnt.cur().lat, 
                    pnt.cur().lng
                ) < 8.0;
            
            if (in_pnt) {
                u8g2.drawCircle(32, 31, 10);
                u8g2.drawCircle(32, 31, 15);
            }
            else {
                double courseto = 
                    TinyGPSPlus::courseTo(
                        gps.location.lat(),
                        gps.location.lng(),
                        pnt.cur().lat, 
                        pnt.cur().lng
                    );
                drawPointArrow(u8g2, DEG_TO_RAD*(courseto-gps.course.deg()));
            }
            
            u8g2.drawDisc(32, 31, 6);
            u8g2.setFont(u8g2_font_helvB08_tr);
            u8g2.setDrawColor(0);
            u8g2.drawGlyph(30,36, '0' + pnt.num());
            u8g2.setDrawColor(1);
        }
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки всей информации о GPS
 * ------------------------------------------------------------------------------------------- */
//uint16_t testDeg();

static void displayGPS(U8G2 &u8g2) {
    auto &gps = gpsGet();
    char s[50];
    
    drawTrackState(u8g2);
    
    displayCompasFull(u8g2);
    if (!gps.satellites.isValid() || (gps.satellites.value() == 0))
        return;
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // количество спутников в правом верхнем углу
    sprintf_P(s, PSTR("sat: %d"), gps.satellites.value());
    u8g2.drawStr(65, 8, s);

    // Текущие координаты
    if (gps.location.isValid()) {
        sprintf_P(s, PSTR("la:%f"), gps.location.lat());
        u8g2.drawStr(65, 22, s);
        sprintf_P(s, PSTR("lo:%f"), gps.location.lng());
        u8g2.drawStr(65, 34, s);
    }

    if (gps.location.isValid() && pnt.numValid() && pnt.cur().used) {
        double dist = 
            TinyGPSPlus::distanceBetween(
                gps.location.lat(),
                gps.location.lng(),
                pnt.cur().lat, 
                pnt.cur().lng
            );
        
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
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Высоты (большими цифрами)
 * ------------------------------------------------------------------------------------------- */
static void displayAlt(U8G2 &u8g2) {
    char s[20];
    auto &ac = altCalc();
    
    drawTrackState(u8g2);

    u8g2.setFont(u8g2_font_fub49_tn);
    sprintf_P(s, PSTR("%0.1f"), ac.alt() / 1000);
    //uint8_t x = 17; // Для немоноширных шрифтов автоцентровка не подходит (прыгает при смене цифр)
    //uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2;
    uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))-15;
    u8g2.setCursor(x, 52);
    u8g2.print(s);

    u8g2.setFont(u8g2_font_ncenB08_tr); 
    sprintf_P(s, PSTR("%d km/h"), round(ac.speed() * 3.6));
    u8g2.drawStr(0, 64, s);
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Высоты + GPS
 * ------------------------------------------------------------------------------------------- */
//uint16_t testDeg();

static void displayAltGPS(U8G2 &u8g2) {
    auto &gps = gpsGet();
    char s[50];
    
    drawTrackState(u8g2);
    
    displayCompasFull(u8g2);
    
    u8g2.drawHLine(64, 31, 64);
    
    // Высота и скорость снижения
    auto &ac = altCalc();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    int16_t alt = round(ac.alt());
    int16_t o = alt % ALT_STEP;
    alt -= o;
    if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
    
    u8g2.setFont(u8g2_font_fub20_tn);
    sprintf_P(s, PSTR("%d"), alt);
    u8g2.drawStr(128-u8g2.getStrWidth(s), 20, s);
    
    u8g2.setFont(u8g2_font_helvB08_tf);
    sprintf_P(s, PSTR("%0.1f m/s"), ac.speed());
    u8g2.drawStr(64, 30, s);
    
    // Текущий режим высоты
    u8g2.setFont(u8g2_font_b10_b_t_japanese1);
    switch (ac.state()) {
        case ACST_INIT:         strcpy_P(s, PSTR("INIT"));  break;
        case ACST_GROUND:       strcpy_P(s, PSTR("gnd"));   break;
        case ACST_TAKEOFF40:    strcpy_P(s, PSTR("TO40"));  break;
        case ACST_TAKEOFF:      strcpy_P(s, PSTR("TOFF"));  break;
        case ACST_FREEFALL:     strcpy_P(s, PSTR("FF"));    break;
        case ACST_CANOPY:       strcpy_P(s, PSTR("CNP"));   break;
        case ACST_LANDING:      strcpy_P(s, PSTR("LAND"));  break;
        default: s[0] = '\0';
    }
    u8g2.drawStr(128-u8g2.getStrWidth(s), 30, s);
    
    // Далее жпс данные
    if (!gps.satellites.isValid() || (gps.satellites.value() == 0))
        return;
    
    // 
    if (gps.location.isValid() && pnt.numValid() && pnt.cur().used) {
        double dist = 
            TinyGPSPlus::distanceBetween(
                gps.location.lat(),
                gps.location.lng(),
                pnt.cur().lat, 
                pnt.cur().lng
            );
        
        u8g2.setFont(u8g2_font_fub20_tf);
        if (dist < 950) 
            sprintf_P(s, PSTR("%0.0fm"), dist);
        else if (dist < 9500) 
            sprintf_P(s, PSTR("%0.1fkm"), dist/1000);
        else if (dist < 950000) 
            sprintf_P(s, PSTR("%0.0fkm"), dist/1000);
        else
            sprintf_P(s, PSTR("%0.2fMm"), dist/1000000);
        u8g2.drawStr(128-u8g2.getStrWidth(s), 54, s);
    }
    
    if (gps.speed.isValid()) {
        u8g2.setFont(u8g2_font_helvB08_tf);
        sprintf_P(s, PSTR("%0.1f m/s"), gps.speed.mps());
        u8g2.drawStr(64, 64, s);
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Текущего времени
 * ------------------------------------------------------------------------------------------- */
static void displayTime(U8G2 &u8g2) {
    char s[20];
    
    drawTrackState(u8g2);
    
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
        case MODE_MAIN_ALT:     displayHnd(displayAlt); break;
        case MODE_MAIN_ALTGPS:  displayHnd(displayAltGPS); break;
        case MODE_MAIN_TIME:    displayHnd(displayTime); break;
    }
}

static void btnSelSmpl() {  // Обработка нажатия на среднюю кнопку
    mode ++;
    if (mode > MODE_MAIN_MAX) mode = MODE_MAIN_MIN;
    Serial.print(F("main next: "));
    Serial.println(mode);
    inf.set().mainmode = mode;
    inf.save();
    displayHnd();
    timerUpdate(MAIN_TIMEOUT);
}

/* ------------------------------------------------------------------------------------------- *
 *  Включение/выключение трэкинга
 * ------------------------------------------------------------------------------------------- */
static void trackTgl() {
    if (trkRunning())
        trkStop();
    else
        trkStart(true);
}

/* ------------------------------------------------------------------------------------------- *
 *  Автопереключение главного экрана при смене режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void autoChgMode(int8_t m) {
    switch (m) {
        //case MODE_MAIN_NONE      0
        //case MODE_MAIN_LAST      -1
        case MODE_MAIN_GPS:
        case MODE_MAIN_ALT:
        case MODE_MAIN_ALTGPS:
        case MODE_MAIN_TIME:
            mode = m;
            displayHnd();
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Таймаут бездействия в основном режиме
 * ------------------------------------------------------------------------------------------- */
static void mainTimeout() {
    if (altCalc().state() != ACST_GROUND)
        return;
    autoChgMode(cfg.d().dsplgnd);
    if (inf.d().mainmode != mode) {
        inf.set().mainmode = mode;
        inf.save();
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Обработка изменения режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_state_t prev, ac_state_t curr) {
    if ((prev == ACST_FREEFALL) && (curr != ACST_FREEFALL) && cfg.d().dsplautoff) {
        // Восстанавливаем обработчики после принудительного FF-режима
        modeMain();
    }
    
    if (modemain)
        timerHnd(mainTimeout, MAIN_TIMEOUT);
    
    switch (curr) {
        case ACST_GROUND:
            autoChgMode(cfg.d().dsplland);
            break;
        
        case ACST_FREEFALL:
            if (cfg.d().dsplautoff) {
                mode = MODE_MAIN_ALT;
                displayHnd();
                btnHndClear(); // Запрет использования кнопок в принудительном FF-режиме
            }
            break;
        
        case ACST_CANOPY:
            autoChgMode(cfg.d().dsplcnp);
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация главного режима отображения показаний
 * ------------------------------------------------------------------------------------------- */
void modeMain() {
    modemain = true;
    displayHnd();
    btnHndClear();
    btnHnd(BTN_UP,  BTN_LONG,   displayLightTgl);
    btnHnd(BTN_SEL, BTN_SIMPLE, btnSelSmpl);
    btnHnd(BTN_SEL, BTN_LONG,   modeMenu);
    btnHnd(BTN_DOWN,BTN_LONG,   trackTgl);
    altStateHnd(altState);
    
    timerHnd(mainTimeout, MAIN_TIMEOUT);
    
    Serial.println(F("mode main"));
}

void initMain(int8_t m) {
    mode = m;
    modeMain();
}

void setModeMain(int8_t m) {
    mode = m;
}

void modeFF() {
    altState(ACST_FREEFALL, ACST_FREEFALL);
}
