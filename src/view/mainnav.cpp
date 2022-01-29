
#include "main.h"

#include "../gps/compass.h"
#include "../gps/proc.h"
#include "../cfg/point.h"
#include "../jump/proc.h"

/* ------------------------------------------------------------------------------------------- *
 *  Навигация
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          static_cast<int>(round(cos(ang) * ((x) - (cx)) - sin(ang) * ((y) - (cy))) + (cx)),\
                                    static_cast<int>(round(sin(ang) * ((x) - (cx)) + cos(ang) * ((y) - (cy))) + (cy))

#define ARG_DEF   U8G2 &u8g2, int cx, int cy, int w, int h
#define ARG_CALL  u8g2, cx, cy, w, h

typedef struct { int x, y; } pnt_t;

static void drawBase(ARG_DEF) {
    u8g2.drawDisc(cx, cy, 2);
    
    int rad = h-20;
    int
        f = 1-rad,
        x = 0,
        y = rad,
        dx = 1,
        dy = (0-rad)*2;
    
    u8g2.drawPixel(cx-x, cy-y);
    u8g2.drawPixel(cx-y, cy-x);
    u8g2.drawPixel(cx+x, cy-y);
    u8g2.drawPixel(cx+y, cy-x);
    
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
        u8g2.drawPixel(cx-x, cy-y);
        u8g2.drawPixel(cx-y, cy-x);
        u8g2.drawPixel(cx+x, cy-y);
        u8g2.drawPixel(cx+y, cy-x);
    }
    
    // Высота
    char s[50];
    auto &ac = altCalc();
    int16_t alt = round(ac.alt() + cfg.d().altcorrect);
    int16_t o = alt % ALT_STEP;
    alt -= o;
    if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;
    sprintf_P(s, PSTR("%d"), alt);
    
    if (w > 100)
        u8g2.setFont(u8g2_font_fub20_tf);
    else
        u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(0, u8g2.getAscent(), s);
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

static void drawPoint(ARG_DEF, double head = 0) {
    auto &gps = gpsInf();
    char s[50];
    
    double dist =
        gpsDistance(
            GPS_LATLON(gps.lat),
            GPS_LATLON(gps.lon),
            pnt.cur().lat, 
            pnt.cur().lng
        );
    
    if (dist < 950) 
        sprintf_P(s, PSTR("%0.0fm"), dist);
    else if (dist < 9500) 
        sprintf_P(s, PSTR("%0.1fk"), dist/1000);
    else if (dist < 950000) 
        sprintf_P(s, PSTR("%0.0fk"), dist/1000);
    else
        sprintf_P(s, PSTR("%0.2fM"), dist/1000000);
    if (w > 100)
        u8g2.setFont(u8g2_font_fub20_tf);
    else
        u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(w-u8g2.getStrWidth(s), u8g2.getAscent(), s);
    
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
    
    int prad = dist > 1000 ? h-20 : dist * (h-20) / 1000;
    drawPntC(u8g2, PNT(cx,cy-prad, courseto, cx,cy));
}

static void drawMoveArr(ARG_DEF, int n = 0, double head = 0) {
    u8g2.drawTriangle(
#if HWVER < 4
        PNT(cx-5,cy-(n*6), head, cx,cy),
        PNT(cx,cy-(n*6)-6, head, cx,cy),
        PNT(cx+5,cy-(n*6), head, cx,cy)
#else // if HWVER < 4
        PNT(cx-10,cy-(n*12), head, cx,cy),
        PNT(cx,cy-(n*12)-12, head, cx,cy),
        PNT(cx+10,cy-(n*12), head, cx,cy)
#endif
    );
}

static void drawMove(ARG_DEF, double head = 0) {
    auto &gps = gpsInf();
    head += DEG_TO_RAD*GPS_DEG(gps.heading);
    
    double speed = GPS_CM(gps.gSpeed);
    
    if (speed < 1.5)
        return;
    drawMoveArr(ARG_CALL, 0, head);
    if (speed < 5)
        return;
    drawMoveArr(ARG_CALL, 1, head);
    if (speed < 12)
        return;
    drawMoveArr(ARG_CALL, 2, head);
    if (speed < 22)
        return;
    drawMoveArr(ARG_CALL, 3, head);
    if (speed < 35)
        return;
    drawMoveArr(ARG_CALL, 4, head);
}

static void drawNavi(ARG_DEF, double head = 0) {
    auto &gps = gpsInf();

    if (!GPS_VALID(gps)) {
        u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
        u8g2.drawGlyph(cx-16,cy+3, 'J');
        return;
    }
    
    if (GPS_VALID_LOCATION(gps) && pnt.numValid() && pnt.cur().used)
        drawPoint(ARG_CALL, head);
    
    if (GPS_VALID_LOCATION(gps) && GPS_VALID_HEAD(gps) && GPS_VALID_SPEED(gps))
        drawMove(ARG_CALL, head);
}

static void drawCompass(ARG_DEF, double head = 0) {
    int rad = h-20;
    pnt_t p;
    u8g2.setFont(u8g2_font_helvB08_tr);
    
    p = { PNT(cx-4,cy-rad-4, head, cx-4,cy+4) };
    if (p.y >= h) p.y = h-1;
    u8g2.drawGlyph(p.x, p.y, 'N');
    
    //p = { PNT(cx, 20, head, cx, cy) };
    //if (p.y > cy+2) p.y = cy+2;
    //u8g2.drawDisc(p.x, p.y, 2);
    
    drawNavi(ARG_CALL, head);
}

class ViewMainNav : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_ALT);
        }
        
        void draw(U8G2 &u8g2) {
            int w = u8g2.getDisplayWidth();
            int h = u8g2.getDisplayHeight();
            int cx = w/2-1, cy = h-12;
            
            drawBase(ARG_CALL);
            
            drawCompass(ARG_CALL, 2*PI - compass().head);
        }
};
static ViewMainNav vNav;
void setViewMainNav() { viewSet(vNav); }
