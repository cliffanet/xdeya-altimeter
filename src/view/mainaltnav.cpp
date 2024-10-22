
#include "main.h"

#include "../navi/compass.h"
#include "../navi/proc.h"
#include "../jump/proc.h"
#include "../jump/track.h"
#include "../cfg/main.h"
#include "../cfg/point.h"

/* ------------------------------------------------------------------------------------------- *
 *  Навигация + Высотомер
 * ------------------------------------------------------------------------------------------- */
static inline pnt_t pntinit(int cx, int cy, int rx, int ry, double ang) {
    // "PI/2-ang"   - это смещение математического угла до географического
    //                  + преобразование, т.к. математический угол движется против часовой стрелки,
    //                  а географический - по часовой
    // "cy - ..."   - это разница между матиматической осью o-y (снизу вверх) и дисплейной (сверху вниз)
    return {
        static_cast<int>(cx + round(cos(PI/2-ang) * rx)),
        static_cast<int>(cy - round(sin(PI/2-ang) * ry))
    };
}

static inline pnt_t pntinit(int cx, int cy, const pnt_t &p, double ang) {
    // Вращение точки вокруг центра _по_ часовой стрелке
    //
    // В математике OY направлена вверх, а градусы двигаются против часовой стрелки,
    // т.е. от оси OX через положительное направление OY.
    // У нас вращение географическое (по часовой стрелке), но и ось OY направлена вниз,
    // т.е. нужное нам вращение будет так же через положительное направление OY,
    // поэтому нам подходит оригинальная математическая формула вращения точки:
    //      x2 = x1*cos - y1*sin
    //      y2 = x1*sin + y1*cos
    
    // Надо:
    // - из точки вычесть координаты центра вращения,
    // - провращать точку,
    // - обратно прибавить координаты точки вращения.
    return {
        static_cast<int>(cx + round(cos(ang) * (p.x - cx) - sin(ang) * (p.y - cy))),
        static_cast<int>(cy + round(sin(ang) * (p.x - cx) + cos(ang) * (p.y - cy)))
    };
}

#define PNT(r,ang)          pntinit(cx,cy, r, r, ang)
#define XY(p)               p.x, p.y
#define P2XY                p.x, p.y

#define ARG_COMP_DEF   U8G2 &u8g2, int cx, int cy, int w, int h
#define ARG_COMP_CALL  u8g2, cx, cy, w, h

#if HWVER < 4

static void drawText(ARG_COMP_DEF) {
    char s[50];
    
    u8g2.drawHLine(u8g2.getDisplayWidth()-64, 31, 64);

    // Высота и скорость снижения
    auto &ac = altCalc();
    if (ac.state() > ACST_INIT) {
        u8g2.setFont(u8g2_font_fub20_tn);
        ViewMain::drawAlt(u8g2, -1, 20);
        
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

    // Расстояние до точки
    u8g2.setFont(u8g2_font_fub20_tf);
    ViewMain::drawNavDist(u8g2, 54);

    // гориз скорость
    auto &gps = gpsInf();
    if (gps.validSpeed()) {
        u8g2.setFont(menuFont);
        sprintf_P(s, PTXT(MAIN_3DSPEED_MS), NAV_CM_F(gps.gSpeed));
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
        u8g2.setFont(u8g2_font_logisoso30_tf);
        ViewMain::drawAlt(u8g2, -1, 30);
        
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

    // Расстояние до точки
    u8g2.setFont(u8g2_font_logisoso30_tf);
    ViewMain::drawNavDist(u8g2, 81);

    // гориз скорость
    auto &gps = gpsInf();
    if (gps.validSpeed()) {
        u8g2.setFont(u8g2_font_ImpactBits_tr);
        sprintf_P(s, PSTR("%0.1f m/s"), NAV_CM_F(gps.gSpeed));
        u8g2.drawStr(u8g2.getDisplayWidth()-64, 95, s);
    }

    // Качество
    if (
            (ac.state() > ACST_INIT) && 
            (ac.direct() == ACDIR_DOWN) &&
            gps.validSpeed()
        ) {
        u8g2.setFont(u8g2_font_ImpactBits_tr);
        auto k = NAV_CM_F(gps.gSpeed) / abs(ac.speedapp());
        sprintf_P(s, PSTR("q: %0.1f"), k);
        u8g2.drawTxt(85, u8g2.getDisplayHeight()-1, s);
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

static void drawPntC(U8G2 &u8g2, const pnt_t &p) {
    u8g2.setDrawColor(0);
    u8g2.drawDisc(P2XY, 8);
    u8g2.setDrawColor(1);
    u8g2.drawDisc(P2XY, 6);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setDrawColor(0);
    u8g2.drawGlyph(p.x-2,p.y+4, '0' + pnt.num());
    u8g2.setDrawColor(1);
}

static void drawPoint(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();
    
    double dist =
        gpsDistance(
            gps.getLat(),
            gps.getLon(),
            pnt.cur().lat, 
            pnt.cur().lng
        );
    
    if (dist < 8) {
        // in point
        drawPntC(u8g2, { cx,cy });
        u8g2.drawCircle(cx,cy, 10);
        u8g2.drawCircle(cx,cy, 15);
        return;
    }
    
    double ang =
        gpsCourse(
            gps.getLat(),
            gps.getLon(),
            pnt.cur().lat, 
            pnt.cur().lng
        ) * DEG_TO_RAD + head;
    
    int prad = dist > 200 ? cy-5 : dist * (cy-5) / 200;
    drawPntC(u8g2, PNT(prad, ang));
}


static void drawMoveArr(ARG_COMP_DEF, int n = 0, double ang = 0) {
    pnt_t
#if HWVER < 4
        p1 = pntinit(cx,cy, { cx-3,cy-(n*4) }, ang),
        p2 = pntinit(cx,cy, { cx,cy-(n*4)-4 }, ang),
        p3 = pntinit(cx,cy, { cx+3,cy-(n*4) }, ang);
#else // if HWVER < 4
        p1 = pntinit(cx,cy, { cx-6,cy-(n*7) }, ang),
        p2 = pntinit(cx,cy, { cx,cy-(n*7)-7 }, ang),
        p3 = pntinit(cx,cy, { cx+6,cy-(n*7) }, ang);
#endif
    
    u8g2.drawTriangle(XY(p1), XY(p2), XY(p3));
}


static void drawMove(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();
    double ang = gps.headRad() + head;
    
    double speed = NAV_CM_F(gps.gSpeed);
    
    if (speed < 0.5)
        return;
    drawMoveArr(ARG_COMP_CALL, 0, ang);
    if (speed < 5)
        return;
    drawMoveArr(ARG_COMP_CALL, 1, ang);
    if (speed < 15)
        return;
    drawMoveArr(ARG_COMP_CALL, 2, ang);
    if (speed < 25)
        return;
    drawMoveArr(ARG_COMP_CALL, 3, ang);
}

static void drawNavi(ARG_COMP_DEF, double head = 0) {
    auto &gps = gpsInf();

    if (!gps.valid()) {
        u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
        u8g2.drawGlyph(cx-16,cy+16, 'J');
        return;
    }

    if (!gps.validPoint()) {
        u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
        u8g2.drawGlyph(cx-16,cy+16, 'R');
        return;
    }
    
    if (gps.validLocation() && pnt.numValid() && pnt.cur().used)
        drawPoint(ARG_COMP_CALL, head);
    
    if (gps.validLocation() && gps.validHead() && gps.validSpeed())
        drawMove(ARG_COMP_CALL, head);
}

static void drawCompass(ARG_COMP_DEF, double head = 0) {
    pnt_t p = PNT(cy-5, head);
    
    u8g2.setDrawColor(0);
    u8g2.drawDisc(P2XY, 7);

    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawGlyph(p.x-3, p.y+4, 'N');
    
    drawNavi(ARG_COMP_CALL, head);
}

/* ------------------------------------------------------------------------------------------- *
 *  NAVI + высотомер
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
            drawNavSat(u8g2);
            
            drawCompass(ARG_COMP_CALL, compass().head);
        }
};
static ViewMainAltNav vAltNav;
void setViewMainAltNav() { viewSet(vAltNav); }
