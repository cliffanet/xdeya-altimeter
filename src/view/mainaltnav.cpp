
#include "main.h"

#include "../gps/compass.h"
#include "../gps/proc.h"
#include "../jump/proc.h"
#include "../file/track.h"
#include "../cfg/main.h"
#include "../cfg/point.h"

/* ------------------------------------------------------------------------------------------- *
 *  Навигация + Высотомер
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          static_cast<int>(round(cos(ang) * ((x) - (cx)) - sin(ang) * ((y) - (cy))) + (cx)),\
                                    static_cast<int>(round(sin(ang) * ((x) - (cx)) + cos(ang) * ((y) - (cy))) + (cy))

#define ARG_COMP_DEF   U8G2 &u8g2, int cx, int cy, int w, int h
#define ARG_COMP_CALL  u8g2, cx, cy, w, h



static void drawGpsState(U8G2 &u8g2) {
    auto &gps = gpsInf();
    char s[50];
    
    u8g2.setFont(menuFont);
    
    switch (gpsState()) {
        case GPS_STATE_OFF:
            strcpy_P(s, PTXT(MAIN_GPSSTATE_OFF));
            break;
        
        case GPS_STATE_INIT:
            strcpy_P(s, PTXT(MAIN_GPSSTATE_INIT));
            break;
        
        case GPS_STATE_FAIL:
            strcpy_P(s, PTXT(MAIN_GPSSTATE_INITFAIL));
            break;
        
        case GPS_STATE_NODATA:
            strcpy_P(s, PTXT(MAIN_GPSSTATE_NODATA));
            break;
        
        case GPS_STATE_OK:
            sprintf_P(s, PTXT(MAIN_GPSSTATE_SATCOUNT), gps.numSV);
            break;
    }
    u8g2.drawTxt(0, u8g2.getDisplayHeight()-1, s);
}


#if HWVER < 4

static void drawText(ARG_COMP_DEF) {
    char s[50];
    
    u8g2.drawHLine(u8g2.getDisplayWidth()-64, 31, 64);

    // Высота и скорость снижения
    auto &ac = altCalc();
    if (ac.state() > ACST_INIT) {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        int16_t alt = round(ac.alt() + cfg.d().altcorrect);
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
        u8g2.setFont(menuFont);
        sprintf_P(s, PTXT(MAIN_VSPEED_MS), abs(ac.speedapp()));
        u8g2.drawTxt(u8g2.getDisplayWidth()-58, 30, s);
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

    // навигация
    auto &gps = gpsInf();
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
            sprintf_P(s, PSTR("%0.1fk"), dist/1000);
        else if (dist < 950000) 
            sprintf_P(s, PSTR("%0.0fk"), dist/1000);
        else
            sprintf_P(s, PSTR("%0.2fM"), dist/1000000);
        u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 54, s);
    }

    if (GPS_VALID_SPEED(gps)) {
        u8g2.setFont(menuFont);
        sprintf_P(s, PTXT(MAIN_3DSPEED_MS), GPS_CM(gps.gSpeed));
        u8g2.drawTxt(u8g2.getDisplayWidth()-64, 64, s);
    }
}

#else // if HWVER < 4

static void drawText(ARG_COMP_DEF) {
    char s[50];
    
    u8g2.drawHLine(u8g2.getDisplayWidth()-64, 47, 64);

    // Высота и скорость снижения
    auto &ac = altCalc();
    if (ac.state() > ACST_INIT) {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        int16_t alt = round(ac.alt() + cfg.d().altcorrect);
        int16_t o = alt % ALT_STEP;
        alt -= o;
        if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;

        u8g2.setFont(u8g2_font_logisoso30_tf);
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

    // навигация
    auto &gps = gpsInf();
    if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used) {
        double dist = 
            gpsDistance(
                GPS_LATLON(gps.lat),
                GPS_LATLON(gps.lon),
                pnt.cur().lat, 
                pnt.cur().lng
            );

        u8g2.setFont(u8g2_font_logisoso30_tf);
        if (dist < 950) 
            sprintf_P(s, PSTR("%0.0fm"), dist);
        else if (dist < 9500) 
            sprintf_P(s, PSTR("%0.1fk"), dist/1000);
        else if (dist < 950000) 
            sprintf_P(s, PSTR("%0.0fk"), dist/1000);
        else
            sprintf_P(s, PSTR("%0.2fM"), dist/1000000);
        u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), 81, s);
    }

    if (GPS_VALID_SPEED(gps)) {
        u8g2.setFont(u8g2_font_ImpactBits_tr);
        sprintf_P(s, PSTR("%0.1f m/s"), GPS_CM(gps.gSpeed));
        u8g2.drawStr(u8g2.getDisplayWidth()-64, 95, s);
    }
}

#endif // if HWVER < 4



static void drawCircleSeg(U8G2 &u8g2, int x, int y, int cx, int cy) {
    u8g2.drawPixel(cx-x, cy-y);
    u8g2.drawPixel(cx-y, cy-x);
    u8g2.drawPixel(cx+x, cy-y);
    u8g2.drawPixel(cx+y, cy-x);
    u8g2.drawPixel(cx+x, cy+y);
    u8g2.drawPixel(cx+y, cy+x);
    u8g2.drawPixel(cx-x, cy+y);
    u8g2.drawPixel(cx-y, cy+x);
}

static void drawGrid(ARG_COMP_DEF) {
#if HWVER < 4
    u8g2.drawPixel(cx, cy);
#else
    u8g2.drawDisc(cx, cy, 2);
#endif
    
    int rad = cy-5;
    int
        f = 1-rad,
        x = 0,
        y = rad,
        dx = 1,
        dy = (0-rad)*2;
    
    drawCircleSeg(u8g2, x, y, cx, cy);
    
    while (x < y) {
        if (f > 0) {
            y --;
            dy += 2;
            f += dy;
        }
        
        x ++;
        dx += 2;
        f += dx;
        if ((x & 0x3) != 0x3) // для каждого 0, 1, 2, 3 - пропускаем 0, 1 и 2
            continue;
    
        drawCircleSeg(u8g2, x, y, cx, cy);
    }
}

static void drawPntC(U8G2 &u8g2, int x, int y) {
    u8g2.setDrawColor(0);
    u8g2.drawDisc(x,y, 8);
    u8g2.setDrawColor(1);
    u8g2.drawDisc(x,y, 6);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setDrawColor(0);
    u8g2.drawGlyph(x-2,y+4, '0' + pnt.num());
    u8g2.setDrawColor(1);
}

static void drawPoint(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();
    
    double dist =
        gpsDistance(
            GPS_LATLON(gps.lat),
            GPS_LATLON(gps.lon),
            pnt.cur().lat, 
            pnt.cur().lng
        );
    
    if (dist < 8) {
        // in point
        drawPntC(u8g2, cx,cy);
        u8g2.drawCircle(cx,cy, 10);
        u8g2.drawCircle(cx,cy, 15);
        return;
    }
    
    double courseto =
        gpsCourse(
            GPS_LATLON(gps.lat),
            GPS_LATLON(gps.lon),
            pnt.cur().lat, 
            pnt.cur().lng
        ) * DEG_TO_RAD;
    courseto += head;
    
    int prad = dist > 500 ? cy-5 : dist * (cy-5) / 500;
    drawPntC(u8g2, PNT(cx,cy-prad, courseto, cx,cy));
}

static void drawMoveArr(ARG_COMP_DEF, int n = 0, double head = 0) {
    u8g2.drawTriangle(
#if HWVER < 4
        PNT(cx-3,cy-(n*4), head, cx,cy),
        PNT(cx,cy-(n*4)-4, head, cx,cy),
        PNT(cx+3,cy-(n*4), head, cx,cy)
#else // if HWVER < 4
        PNT(cx-6,cy-(n*7), head, cx,cy),
        PNT(cx,cy-(n*7)-7, head, cx,cy),
        PNT(cx+6,cy-(n*7), head, cx,cy)
#endif
    );
}

static void drawMove(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();
    head += DEG_TO_RAD*GPS_DEG(gps.heading);
    
    double speed = GPS_CM(gps.gSpeed);
    
    if (speed < 1.5)
        return;
    drawMoveArr(ARG_COMP_CALL, 0, head);
    if (speed < 7)
        return;
    drawMoveArr(ARG_COMP_CALL, 1, head);
    if (speed < 18)
        return;
    drawMoveArr(ARG_COMP_CALL, 2, head);
    if (speed < 30)
        return;
    drawMoveArr(ARG_COMP_CALL, 3, head);
}

static void drawNavi(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();

    if (!GPS_VALID(gps)) {
        u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
        u8g2.drawGlyph(cx-16,cy+16, 'J');
        return;
    }
    
    if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used)
        drawPoint(ARG_COMP_CALL, head);
    
    if (GPS_VALID_LOCATION(gps) && GPS_VALID_HEAD(gps) && GPS_VALID_SPEED(gps))
        drawMove(ARG_COMP_CALL, head);
}

static void drawCompass(ARG_COMP_DEF, double head = 0) {
    int rad = cy-10;
    pnt_t p;
    
    p = { PNT(cx,cy-rad-5, head, cx,cy) };
    if (p.y > h-7) p.y = h-7;
    u8g2.setDrawColor(0);
    u8g2.drawDisc(p.x, p.y, 7);

    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawGlyph(p.x-3, p.y+4, 'N');
    
    drawNavi(ARG_COMP_CALL, head);
}

/* ------------------------------------------------------------------------------------------- *
 *  GPS + высотомер
 * ------------------------------------------------------------------------------------------- */

class ViewMainAltNav : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_NAV);
        }
        
        void draw(U8G2 &u8g2) {
            int w = u8g2.getDisplayWidth();
            int h = u8g2.getDisplayHeight();
            int cx = h/2-1, cy = h/2;
            
#if HWVER < 4
            //cx += 5;
#else
            cx += 15;
#endif
    
            drawState(u8g2);
            drawClock(u8g2);
            
            drawGrid(ARG_COMP_CALL);
            drawText(ARG_COMP_CALL); // Весь текст пишем в самом начале, чтобы графика рисовалась поверх
            drawGpsState(u8g2);
            
            drawCompass(ARG_COMP_CALL, 2*PI - compass().head);
        }
};
static ViewMainAltNav vAltNav;
void setViewMainAltNav() { viewSet(vAltNav); }
