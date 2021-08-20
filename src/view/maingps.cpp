
#include "main.h"

#include "../gps/proc.h"
#include "../jump/proc.h"
#include "../file/track.h"
#include "../cfg/point.h"



#if HWVER < 4
#define COMP_X      32
#define COMP_Y      31
#define COMP_R      
#else // if HWVER < 4
#define COMP_X      48
#define COMP_Y      47
#define COMP_R      
#endif // if HWVER < 4

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки компаса, провёрнутого на угол ang
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          round(cos(ang) * (x - cx) - sin(ang) * (y - cy)) + cx,  round(sin(ang) * (x - cx) + cos(ang) * (y - cy)) + cy
// APNT - упрощённая версия PNT, где ANG берётся автоматически из текущей зоны видимости кода
#define APNT(x,y,cx,cy)             PNT(x,y,ang,cx,cy)
inline void drawCompas(U8G2 &u8g2, float ang) {
    u8g2.setFont(u8g2_font_helvB08_tr);
    
    u8g2.drawDisc(COMP_X, COMP_Y, 1);
    //u8g2.drawCircle(32,31, 31);
    u8g2.drawDisc(APNT(COMP_X-1,6,COMP_X-1,COMP_Y+1), 7);
    u8g2.setDrawColor(0);
    u8g2.drawGlyph(APNT(COMP_X-4,10,COMP_X-4,COMP_Y+5), 'N');
    u8g2.setDrawColor(1);
    u8g2.drawGlyph(APNT(COMP_X-4,(COMP_X+1)*2-1,COMP_X-4,COMP_Y+5), 'S');
    u8g2.drawGlyph(APNT(1,COMP_Y+5,COMP_X-4,COMP_Y+5), 'W');
    u8g2.drawGlyph(APNT(COMP_X*2-7,COMP_Y+5,COMP_X-4,COMP_Y+5), 'E');
}

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки стрелки направления к точке внутри компаса
 * ------------------------------------------------------------------------------------------- */
inline void drawPointArrow(U8G2 &u8g2, float ang) {
    //u8g2.drawLine(APNT(32,15,32,31), APNT(32,42,32,31));
    u8g2.drawLine(APNT(COMP_X,15,COMP_X,COMP_Y), APNT(COMP_X-2,COMP_Y+5,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,15,COMP_X,COMP_Y), APNT(COMP_X+3,COMP_Y+5,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X-2,COMP_Y+5,COMP_X,COMP_Y), APNT(COMP_X+2,COMP_Y+5,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,15,COMP_X,COMP_Y), APNT(COMP_X-4,COMP_Y-11,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,15,COMP_X,COMP_Y), APNT(COMP_X+4,COMP_Y-11,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,14,COMP_X,COMP_Y), APNT(COMP_X-5,COMP_Y-11,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,14,COMP_X,COMP_Y), APNT(COMP_X+5,COMP_Y-11,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,16,COMP_X,COMP_Y), APNT(COMP_X-4,COMP_Y-10,COMP_X,COMP_Y));
    u8g2.drawLine(APNT(COMP_X,16,COMP_X,COMP_Y), APNT(COMP_X+4,COMP_Y-10,COMP_X,COMP_Y));
}

static void displayCompasFull(U8G2 &u8g2) {
    auto &gps = gpsInf();

    if (!GPS_VALID(gps)) {
        u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
        u8g2.drawGlyph(16, 48, 'J');
        return;
    }
    
    // Компас и стрелка к точке внутри него
    if (GPS_VALID_LOCATION(gps) && GPS_VALID_HEAD(gps)) {
        // Компас показывает, куда смещено направление нашего движения 
        // относительно сторон Света,
        drawCompas(u8g2, DEG_TO_RAD*(360 - GPS_DEG(gps.heading)));
        // а стрелка показывает отклонение направления к точке относительно 
        // направления нашего движения
        if (pnt.numValid() && pnt.cur().used) {
            bool in_pnt = 
                gpsDistance(
                    GPS_LATLON(gps.lat),
                    GPS_LATLON(gps.lon),
                    pnt.cur().lat, 
                    pnt.cur().lng
                ) < 8.0;
            
            if (in_pnt) {
                u8g2.drawCircle(COMP_X, COMP_Y, 10);
                u8g2.drawCircle(COMP_X, COMP_Y, 15);
            }
            else {
                double courseto = 
                    gpsCourse(
                        GPS_LATLON(gps.lat),
                        GPS_LATLON(gps.lon),
                        pnt.cur().lat, 
                        pnt.cur().lng
                    );
                drawPointArrow(u8g2, DEG_TO_RAD*(courseto-GPS_DEG(gps.heading)));
            }
            
            u8g2.drawDisc(COMP_X, COMP_Y, 6);
            u8g2.setFont(u8g2_font_helvB08_tr);
            u8g2.setDrawColor(0);
            u8g2.drawGlyph(COMP_X-2,COMP_X+5, '0' + pnt.num());
            u8g2.setDrawColor(1);
        }
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Только GPS
 * ------------------------------------------------------------------------------------------- */
/*
class ViewMainGps : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            
            setViewMain(MODE_MAIN_ALT);
        }
        void draw(U8G2 &u8g2) {
            auto &gps = gpsInf();
            char s[50];
    
            drawState(u8g2);
            drawClock(u8g2);
    
            displayCompasFull(u8g2);
            if (gps.numSV == 0)
                return;
    
            u8g2.setFont(u8g2_font_ncenB08_tr);
    
            // количество спутников в правом верхнем углу
            sprintf_P(s, PSTR("s: %d/%d"), gps.numSV, gps.gpsFix);
            u8g2.drawStr(65, 8, s);

            // Текущие координаты
            if (GPS_VALID_LOCATION(gps)) {
                sprintf_P(s, PSTR("la:%f"), GPS_LATLON(gps.lat));
                u8g2.drawStr(65, 22, s);
                sprintf_P(s, PSTR("lo:%f"), GPS_LATLON(gps.lon));
                u8g2.drawStr(65, 34, s);
            }

            if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used) {
                double dist = 
                    gpsDistance(
                        GPS_LATLON(gps.lat),
                        GPS_LATLON(gps.lon),
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
};
static ViewMainGps vGps;
void setViewMainGps() { viewSet(vGps); }
*/
/* ------------------------------------------------------------------------------------------- *
 *  GPS + высотомер
 * ------------------------------------------------------------------------------------------- */

class ViewMainGpsAlt : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_ALT);
        }
        
        void draw(U8G2 &u8g2) {
            auto &gps = gpsInf();
            char s[50];
    
            drawState(u8g2);
            drawClock(u8g2);
    
            displayCompasFull(u8g2);

#if HWVER < 4
            u8g2.drawHLine(u8g2.getDisplayWidth()-64, 31, 64);
    
            // Высота и скорость снижения
            auto &ac = altCalc();
            if (ac.state() > ACST_INIT) {
                u8g2.setFont(u8g2_font_ncenB08_tr);
                int16_t alt = round(ac.alt());
                int16_t o = alt % ALT_STEP;
                alt -= o;
                if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
    
                u8g2.setFont(u8g2_font_fub20_tn);
                sprintf_P(s, PSTR("%d"), alt);
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 20, s);
                
                switch (ac.direct()) {
                    case ACDIR_UP:
                        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                        u8g2.drawGlyph(u8g2.getDisplayWidth()-64, 30, 0x43);
                        break;
                    
                    case ACDIR_DOWN:
                        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                        u8g2.drawGlyph(u8g2.getDisplayWidth()-64, 30, 0x40);
                        break;
                }
                u8g2.setFont(u8g2_font_helvB08_tf);
                sprintf_P(s, PSTR("%0.1f m/s"), abs(ac.speedapp()));
                u8g2.drawStr(u8g2.getDisplayWidth()-58, 30, s);
            }
    
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
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 30, s);
    
            // Далее жпс данные
            if (!GPS_VALID(gps))
                return;
    
            // 
            if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used) {
                double dist = 
                    gpsDistance(
                        GPS_LATLON(gps.lat),
                        GPS_LATLON(gps.lon),
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
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 54, s);
            }
    
            if (GPS_VALID_SPEED(gps)) {
                u8g2.setFont(u8g2_font_helvB08_tf);
                sprintf_P(s, PSTR("%0.1f m/s"), GPS_CM(gps.gSpeed));
                u8g2.drawStr(u8g2.getDisplayWidth()-64, 64, s);
            }
#else // if HWVER < 4
            u8g2.drawHLine(u8g2.getDisplayWidth()-64, 47, 64);
    
            // Высота и скорость снижения
            auto &ac = altCalc();
            if (ac.state() > ACST_INIT) {
                u8g2.setFont(u8g2_font_ncenB08_tr);
                int16_t alt = round(ac.alt());
                int16_t o = alt % ALT_STEP;
                alt -= o;
                if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
    
                u8g2.setFont(u8g2_font_fub30_tn);
                sprintf_P(s, PSTR("%d"), alt);
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 30, s);
                
                switch (ac.direct()) {
                    case ACDIR_UP:
                        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                        u8g2.drawGlyph(u8g2.getDisplayWidth()-64, 44, 0x43);
                        break;
                    
                    case ACDIR_DOWN:
                        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                        u8g2.drawGlyph(u8g2.getDisplayWidth()-64, 44, 0x40);
                        break;
                }
                u8g2.setFont(u8g2_font_ImpactBits_tr);
                sprintf_P(s, PSTR("%0.1f m/s"), abs(ac.speedapp()));
                u8g2.drawStr(u8g2.getDisplayWidth()-58, 44, s);
            }
    
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
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s)+6, 44, s);
    
            // Далее жпс данные
            if (!GPS_VALID(gps))
                return;
    
            // 
            if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used) {
                double dist = 
                    gpsDistance(
                        GPS_LATLON(gps.lat),
                        GPS_LATLON(gps.lon),
                        pnt.cur().lat, 
                        pnt.cur().lng
                    );
        
                u8g2.setFont(u8g2_font_fub30_tf);
                if (dist < 950) 
                    sprintf_P(s, PSTR("%0.0fm"), dist);
                else if (dist < 9500) 
                    sprintf_P(s, PSTR("%0.1fkm"), dist/1000);
                else if (dist < 950000) 
                    sprintf_P(s, PSTR("%0.0fkm"), dist/1000);
                else
                    sprintf_P(s, PSTR("%0.2fMm"), dist/1000000);
                u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 81, s);
            }
    
            if (GPS_VALID_SPEED(gps)) {
                u8g2.setFont(u8g2_font_ImpactBits_tr);
                sprintf_P(s, PSTR("%0.1f m/s"), GPS_CM(gps.gSpeed));
                u8g2.drawStr(u8g2.getDisplayWidth()-64, 95, s);
            }
#endif // if HWVER < 4
        }
};
static ViewMainGpsAlt vGpsAlt;
void setViewMainGpsAlt() { viewSet(vGpsAlt); }
