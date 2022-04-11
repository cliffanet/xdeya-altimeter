
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../filtlib/ring.h"
#include "../navi/compass.h"

#include <cmath>

/* ------------------------------------------------------------------------------------------- *
 *  ViewMagCalib - процесс калибровки магнетометра
 * ------------------------------------------------------------------------------------------- */

class ViewMagCalib : public ViewBase {
    public:
        template <typename T> struct val_t { T a, b, r; };
        
        typedef val_t<int16_t> val16_t;
        typedef val_t<int32_t> val32_t;
        typedef val_t<int64_t> val64_t;
        
        enum {
            ST_Z0,
            ST_0Z,
            ST_X0,
            ST_0X,
            ST_Y0,
            ST_0Y,
            ST_FIN
        };
        
        ViewMagCalib() :
            ViewBase(),
            m_val(6)
        {
            init();
        }
        
        void init() {
            m_val.clear();
            m_avg   = { 0, 0, 0 };
            m_devi  = { 0, 0, 0 };
            m_state = ST_Z0;
            bzero(m_res, sizeof(m_res));
        }
        
        // фоновый процессинг - обрабатываем этапы синхронизации
        void process() {
            if (m_state >= ST_FIN)
                return;
            
            if (
                    (m_val.size() >= 6) &&
                    (m_avg.a < 10) &&
                    (m_avg.b < 10) &&
                    (m_avg.r > 100) &&
                    (m_devi.a < 10) &&
                    (m_devi.b < 10) &&
                    (m_devi.r < 10)
                ) {
                if (m_state < 6)
                    m_res[m_state] = m_avg.r;
                m_state ++;
                m_val.clear();
            }
            
            if (m_state >= ST_FIN)
                compCalibrate(
                    {
                        static_cast<int16_t>(-m_res[ST_X0]),
                        static_cast<int16_t>(-m_res[ST_0Y]),
                        static_cast<int16_t>(-m_res[ST_Z0])
                    },
                    { m_res[ST_0X], m_res[ST_Y0], m_res[ST_0Z] }
                );
            
            val16_t v;
            switch (m_state) {
                case ST_Z0:
                    v = { compass().mag.x, compass().mag.y, static_cast<int16_t>(-compass().mag.z) };
                    break;
                case ST_0Z:
                    v = { compass().mag.x, compass().mag.y, compass().mag.z };
                    break;
                case ST_X0:
                    v = { compass().mag.z, compass().mag.y, static_cast<int16_t>(-compass().mag.x) };
                    break;
                case ST_0X:
                    v = { compass().mag.z, compass().mag.y, compass().mag.x };
                    break;
                case ST_Y0:
                    v = { compass().mag.x, compass().mag.z, compass().mag.y };
                    break;
                case ST_0Y:
                    v = { compass().mag.x, compass().mag.z, static_cast<int16_t>(-compass().mag.y) };
                    break;
                default:
                    v = { 0, 0, 0 };
            }
            m_val.push_back(v);
            
            val64_t avg = { 0, 0, 0 };
            int32_t sign = 0;
            for (const auto &v : m_val) {
                avg.a += v.a * v.a;
                avg.b += v.b * v.b;
                avg.r += v.r * v.r;
                sign += v.r;
            }
            
            m_avg = {
                static_cast<int16_t>(std::sqrt(avg.a / m_val.size())),
                static_cast<int16_t>(std::sqrt(avg.b / m_val.size())),
                static_cast<int16_t>(std::sqrt(avg.r / m_val.size())),
            };
            
            val16_t devi = { 0, 0, 0 };
            for (const auto &v : m_val) {
                val16_t d = {
                    static_cast<int16_t>(abs(m_avg.a - abs(v.a))),
                    static_cast<int16_t>(abs(m_avg.b - abs(v.b))),
                    static_cast<int16_t>(abs(m_avg.r - abs(v.r))),
                };
                if (devi.a < d.a) devi.a = d.a;
                if (devi.b < d.b) devi.b = d.b;
                if (devi.r < d.r) devi.r = d.r;
            }
            m_devi = devi;
            
            if (sign < 0)
                m_avg.r *= -1;
        }
        
        bool useLong(btn_code_t btn) {
            return false;
        }
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            setViewMain();
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            u8g2.setDrawColor(1);
            
            // Прямоугольник
            int16_t sx = m_avg.a;
            if (sx > 200) sx = 200;
            int16_t sy = m_avg.b;
            if (sy > 200) sy = 200;
            
            int16_t smax = u8g2.getDisplayHeight() * 9 / 20;
            sx = sx * smax / 200;
            sy = sy * smax / 200;
            
            u8g2.drawFrame(
                u8g2.getDisplayHeight() / 2 - sx - 1,
                u8g2.getDisplayHeight() / 2 - sy - 1,
                sx * 2 + 2,
                sy * 2 + 2
            );
            
            // список операций
            u8g2.setFont(menuFont);
            const char *tname[] = {
                PSTR("Z - 0"),
                PSTR("0 - Z"),
                PSTR("X - 0"),
                PSTR("0 - X"),
                PSTR("Y - 0"),
                PSTR("0 - Y"),
            };
            for (uint8_t st = ST_Z0, y=10; st < ST_FIN; st++, y+=10) {
                char s[64];
                strcpy_P(s, tname[st]);
                u8g2.drawStr(u8g2.getDisplayWidth()-30, y, s);
                
                if (m_state > st)
                    strcpy_P(s, PSTR("-ok-"));
                else
                if (m_state == st)
                    strcpy_P(s, PSTR("-->"));
                else
                    continue;
                u8g2.drawStr(u8g2.getDisplayWidth()-55, y, s);
            }
            
            char s[64];
            if (m_avg.r < -70) {
                strcpy_P(s, PTXT(MCAL_SIDE180_1));
                u8g2.drawTxt(0, 10, s);
                strcpy_P(s, PTXT(MCAL_SIDE180_2));
                u8g2.drawTxt(0, 20, s);
            }
            else {
                strcpy_P(s, PTXT(MCAL_MINFRAME_1));
                u8g2.drawTxt(0, 10, s);
                strcpy_P(s, PTXT(MCAL_MINFRAME_2));
                u8g2.drawTxt(0, 20, s);
            }
        }
        
    private:
        ring<val16_t> m_val;
        val16_t m_avg;
        val16_t m_devi;
        uint8_t m_state;
        int16_t m_res[6];
};
ViewMagCalib vMagCalib;

void setViewMagCalib() {
    vMagCalib.init();
    viewSet(vMagCalib);
}
