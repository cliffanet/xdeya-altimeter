
#include "main.h"

#include "../navi/proc.h"
#include "../cfg/point.h"
#include "../filtlib/ring.h"
#include "../log.h"

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

public:
    NavPath() : data(3 * 60 * 10) {}

    bool frstmode()     const { return m_frstmode; }

    uint32_t size()     const { return data.size(); }
    size_t valid()      const { return m_valid; }
    bool lastValid()    const {
        return (data.size() > 0) && (data.back().isvalid);
    }

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

    void add(int mode, bool valid, int32_t lon, int32_t lat) {
        bool isnew = m_mode != mode;
        if (isnew) {
            data.clear();
            m_mode = mode;
            m_frstmode = valid;
        }

        if (m_frstmode && (data.size() == data.max_size()))
            m_frstmode = false;
        data.push_back({ valid, lon, lat });

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
    }
};

static NavPath path;

static void drawPath(U8G2 &u8g2) {
    char s[64];
    uint16_t y = 60;
#define prn(fmt, ...) \
            snprintf_P(s, sizeof(s), PSTR(fmt), ##__VA_ARGS__); \
            u8g2.drawStr(5, y, s); \
            y += 10;

    int w = u8g2.getDisplayWidth();
    int h = u8g2.getDisplayHeight();
    // середина дисплея
    const auto dw = w/2, dh = h/2;
    // середина области пути
    const auto  pw = path.width()/2,
                ph = path.height()/2;

    // Подбираем коэфициент масштаба
    double k = 1;
    uint32_t ma = 10; // сколько метров в сантиметре
    for (; ma < 1000000; ma *= 10) {
        // 32 - это примерное число пикселей в 1см на большом дисплее.
        k = static_cast<double>(ma) / 32;
        if (
                ((static_cast<double>(path.width()) / k) <= w) &&
                ((static_cast<double>(path.height())/ k) <= h)
            )
            break;
    }

    NavPath::navmet_t lp = { false };

    for (int i = 0; i < path.size(); i++) {
        auto p = path[i];
        if (!p.isvalid) {
            lp = p;
            continue;
        }

        p.x -= pw;
        p.x = round(static_cast<double>(p.x) / k);
        p.y -= ph;
        p.y = round(static_cast<double>(p.y) / k);

        if (lp.isvalid && ((lp.x != p.x) || (lp.y != p.y)))
            u8g2.drawLine(dw+lp.x, h-(dh+lp.y), dw+p.x, h-(dh+p.y));
        
        lp = p;
    }

    if (lp.isvalid) {
        u8g2.drawCircle(dw+lp.x, h-(dh+lp.y), 2);
        prn("pnt: %dx%d (%d: %.4f)", lp.x, lp.y, ma, k);
    }
#undef prn
}

class ViewMainNavPath : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_ALT);
        }
        
        void draw(U8G2 &u8g2) {
            char s[64];
            uint16_t y = 10;
#define prn(fmt, ...) \
            snprintf_P(s, sizeof(s), PSTR(fmt), ##__VA_ARGS__); \
            u8g2.drawStr(5, y, s); \
            y += 10;

            u8g2.setFont(u8g2_font_ncenB08_tr);
            prn("count:  %u (%u)", path.valid(), path.size());
            prn("width:  %u", path.width());
            prn("height: %u", path.height());
#undef prn

            // ----
            if ((path.width() >= 2) && (path.height() >= 2))
                drawPath(u8g2);
        }
};
static ViewMainNavPath vNavPath;
void setViewMainNavPath() { viewSet(vNavPath); }

void navPathAdd(int mode, bool valid, int32_t lon, int32_t lat) {
    path.add(mode, valid, lon, lat);
}
