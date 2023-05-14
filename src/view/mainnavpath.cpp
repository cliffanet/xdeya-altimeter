
#include "main.h"

#include "../navi/proc.h"
#include "../cfg/point.h"
#include "../jump/proc.h"
#include "../filtlib/ring.h"
#include "../log.h"
#include "../monlog.h"

/* ------------------------------------------------------------------------------------------- *
 *  Навигация - пройденный путь
 * ------------------------------------------------------------------------------------------- */
class NavPath {
    int m_mode = 0;
    size_t m_valid = 0;
    bool m_frstmode = false;
    int32_t m_lonmin = 0;
    int32_t m_lonmax = 0;
    int32_t m_latmin = 0;
    int32_t m_latmax = 0;
    double m_ang = 0;

    typedef struct {
        bool isvalid;
        int32_t lon;
        int32_t lat;
    } navp_t;

    ring<navp_t> data;
    /*
    const navp_t data[8] = {
        { true, 375200530, 558305110 },
        { true, 375199570, 558279190 },
        { true, 375249940, 558279730 },
        { true, 375249310, 558305930 },
        { true, 375234740, 558298450 },
        { true, 375236760, 558284580 },
        { true, 375211630, 558285000 },
        { true, 375210350, 558298280 }
    };
    */
    navp_t m_pnt = { false };

public:
    NavPath() : data(3 * 60 * 10) {}

    bool frstmode()     const { return m_frstmode; }

    uint32_t size()     const { return data.size(); }
    size_t valid()      const { return m_valid; }
    bool lastValid()    const {
        return (data.size() > 0) && (data.back().isvalid);
    }

    double ang()        const { return m_ang; }

    // коэфициенты для преобразования градусов в метры
    uint64_t klon() const {
        const auto latmid = (m_latmax-m_latmin) / 2 + m_latmin;
        const double lc = cos(static_cast<double>(latmid) / NAV_KOEF_LATLON * DEGRAD);
        return 40075700LL / 360 * lc;
    }
    uint64_t klat() const {
        return 20003930LL / 180;
    }

    // ширина и высота в метрах
    uint32_t width() const {
        return klon() * abs(m_lonmax-m_lonmin) / NAV_KOEF_LATLON;
    }
    uint32_t height() const {
        return klat() * abs(m_latmax-m_latmin) / NAV_KOEF_LATLON;
    }

    typedef struct {
        bool isvalid;
        int x;
        int y;
    } navmet_t;

    navmet_t operator[](size_t index) const {
        if (data.empty() || !data[index].isvalid)
            return { false, 0, 0 };
        
        const auto &d = data[index];
        
        return {
            true,
            klon() * (d.lon-m_lonmin) / NAV_KOEF_LATLON,
            klat() * (d.lat-m_latmin) / NAV_KOEF_LATLON,
        };
    }

    navmet_t dstpnt() const {
        return {
            true,
            klon() * (m_pnt.lon-m_lonmin) / NAV_KOEF_LATLON,
            klat() * (m_pnt.lat-m_latmin) / NAV_KOEF_LATLON,
        };
    }

    void add(int mode, bool valid, int32_t lon, int32_t lat, double ang) {
        if (m_mode != mode) {
            CONSOLE("cleared by mode %d (val: %d, prv: %d)", mode, valid, m_mode);
            //MONITOR("clear: %d (val: %d, prv: %d)", mode, valid, m_mode);
            data.clear();
            m_mode = mode;
            m_ang = 0;
        }
        if (data.size() == 0) {
            m_frstmode = valid;
            //MONITOR("m_frstmode beg: %d", m_frstmode);
        }

        if (valid)
            m_ang = ang;

        if (m_frstmode && data.full()) {
            m_frstmode = false;
            //MONITOR("m_frstmode end");
        }
        data.push_back({ valid, lon, lat });

        recalc();
    }

    void recalc() {
        m_valid = 0;
        for (const auto p : data) {
            if (!p.isvalid)
                continue;
            m_valid++;
            if (m_valid <= 1) {
                m_lonmin = p.lon;
                m_lonmax = p.lon;
                m_latmin = p.lat;
                m_latmax = p.lat;
            }
            else {
                if (m_lonmin > p.lon) m_lonmin = p.lon;
                if (m_lonmax < p.lon) m_lonmax = p.lon;
                if (m_latmin > p.lat) m_latmin = p.lat;
                if (m_latmax < p.lat) m_latmax = p.lat;
            }
        }

        m_pnt.isvalid = (m_valid > 0) && pnt.numValid() && pnt.cur().used;
        if (m_pnt.isvalid) {
            m_pnt.lon = round(pnt.cur().lng * NAV_KOEF_LATLON);
            m_pnt.lat = round(pnt.cur().lat * NAV_KOEF_LATLON);
            if (m_lonmin > m_pnt.lon) m_lonmin = m_pnt.lon;
            if (m_lonmax < m_pnt.lon) m_lonmax = m_pnt.lon;
            if (m_latmin > m_pnt.lat) m_latmin = m_pnt.lat;
            if (m_latmax < m_pnt.lat) m_latmax = m_pnt.lat;
        }
    }
};

static NavPath path;

typedef struct {
    uint16_t met;
    const char *txt;
} ma_t;
static const ma_t maall[] = {
    { 100,  PSTR("100m") },
    { 200,  PSTR("200m") },
    { 500,  PSTR("500m") },
    { 1000, PSTR("1 km") },
    { 2000, PSTR("2 km") },
    { 3000, PSTR("3 km") },
    { 5000, PSTR("5 km") },
    { 10000,PSTR("10km") },
    { 20000,PSTR("20km") },
    { 30000,PSTR("30km") },
    { 40000,PSTR("40km") },
    { 50000,PSTR("50km") },
};

#define XY(p)               p.x, p.y
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
static void drawMoveArr(U8G2 &u8g2, int cx, int cy, double ang = 0) {
    pnt_t
        p1 = pntinit(cx,cy, { cx-4,cy }, ang),
        p2 = pntinit(cx,cy, { cx,  cy-7 }, ang),
        p3 = pntinit(cx,cy, { cx+4,cy }, ang);
    
    u8g2.drawTriangle(XY(p1), XY(p2), XY(p3));
}

static void drawPntC(U8G2 &u8g2, const pnt_t &p) {
    u8g2.setDrawColor(0);
    u8g2.drawDisc(XY(p), 8);
    u8g2.setDrawColor(1);
    u8g2.drawDisc(XY(p), 6);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setDrawColor(0);
    u8g2.drawGlyph(p.x-2,p.y+4, '0' + pnt.num());
    u8g2.setDrawColor(1);
}

static void drawMa(U8G2 &u8g2, int x, int y, const ma_t &ma) {
    u8g2.drawLine(x, y-5, x, y);
    u8g2.drawLine(x, y, x+32, y);
    u8g2.drawLine(x+32, y, x+32, y-5);
    char s[10];
    strncpy_P(s, ma.txt, sizeof(s));
    s[sizeof(s)-1] = '\0';
    u8g2.setFont(u8g2_font_b10_b_t_japanese1);
    u8g2.drawStr(x + (32-u8g2.getTxtWidth(s)) / 2, y-2, s);
}

static void drawPath(U8G2 &u8g2) {
    int w = u8g2.getDisplayWidth();
    int h = u8g2.getDisplayHeight();
    // середина дисплея
    const auto dw = w/2, dh = h/2;
    // середина области пути
    const auto  pw = path.width()/2,
                ph = path.height()/2;

    // Подбираем коэфициент масштаба
    double k = 1;
    ma_t ma; // сколько метров в сантиметре
    for (const auto &ma1 : maall) {
        ma = ma1;
        // 32 - это примерное число пикселей в 1см на большом дисплее.
        k = static_cast<double>(ma.met) / 32;
        if (
                ((static_cast<double>(path.width()) / k) <= (w-10)) &&
                ((static_cast<double>(path.height())/ k) <= (h-10))
            )
            break;
    }

    // Функция преобразования координат в экранные
#define pntmap(p) \
        p.x -= pw; \
        p.x = dw + round(static_cast<double>(p.x) / k); \
        p.y -= ph; \
        p.y = dh + round(static_cast<double>(p.y) / k); \
        p.y = h-p.y;

    // Печать масштаба в углу
    drawMa(u8g2, 2, 15, ma);

    // рисуем стартовую точку
    if (path.frstmode() && (path.size() > 0)) {
        auto p = path[0];
        pntmap(p);
        u8g2.setFont(u8g2_font_open_iconic_www_1x_t);
        u8g2.drawGlyph(p.x-4, p.y, 'G');
        //u8g2.drawLine(p.x-3, p.y-3, p.x+3, p.y+3);
        //u8g2.drawLine(p.x+3, p.y-3, p.x-3, p.y+3);
    }

    // Отрисовка пути
    NavPath::navmet_t lp = { false };
    NavPath::navmet_t lv = { false };
    for (int i = 0; i < path.size(); i++) {
        auto p = path[i];

        if (p.isvalid) {
            pntmap(p);
            if (lp.isvalid && ((lp.x != p.x) || (lp.y != p.y)))
                u8g2.drawLine(lp.x, lp.y, p.x, p.y);

            lv = p;
        }
        
        lp = p;
    }

    // Текущая точка
    if (lp.isvalid)
        drawMoveArr(u8g2, lp.x, lp.y, path.ang());
    else
    if (lv.isvalid) {
        u8g2.setFont(u8g2_font_open_iconic_www_2x_t);
        u8g2.drawGlyph(lv.x-8, lv.y+8, 'J');
    }

    // точка-назначение
    auto p = path.dstpnt();
    if (p.isvalid) {
        pntmap(p);
        drawPntC(u8g2, { p.x, p.y });
    }
}

static void drawText(U8G2 &u8g2) {
    // Шрифт для высоты и расстояния
#if HWVER < 4
    u8g2.setFont(u8g2_font_helvB08_tr);
#else // if HWVER < 4
    u8g2.setFont(u8g2_font_fub20_tf);
#endif
    
    // Высота
    ViewMain::drawAlt(u8g2, -1, u8g2.getAscent());
    
    // Расстояние до точки
    ViewMain::drawNavDist(u8g2, u8g2.getDisplayHeight());

    // Количество спутников
    ViewMain::drawNavSat(u8g2);
}

class ViewMainNavPath : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_ALT);
        }
        
        void draw(U8G2 &u8g2) {
            drawText(u8g2);

            if (path.valid() < 10) {
                u8g2.setFont(u8g2_font_open_iconic_www_4x_t);
                u8g2.drawGlyph(u8g2.getDisplayWidth()/2-16, u8g2.getDisplayHeight()/2+16, 'J');
                return;
            }
            // ----
            if ((path.width() >= 2) && (path.height() >= 2))
                drawPath(u8g2);
        }
};
static ViewMainNavPath vNavPath;
void setViewMainNavPath() { viewSet(vNavPath); }

void navPathAdd(int mode, bool valid, int32_t lon, int32_t lat, double ang) {
    path.add(mode, valid, lon, lat, ang);
}
