
#include "main.h"
#include "info.h"
#include "../filtlib/ring.h"
#include "../clock.h"
#include "../monlog.h"

#ifdef FWVER_DEBUG

class ViewMainAltChart : public ViewMain {
    typedef struct {
        int val;
        int avg;
        char mode;
        bool ismode;
        uint16_t mcnt;
    } alt_t;
    ring<alt_t> m_log;
    int m_min = 0, m_max = 0;

    public:
        ViewMainAltChart() : m_log(60*10*2) { }

        void add(int val, int avg) {
            m_log.push_back({ val, avg, '\0', false, 0 });

            m_min = m_log[0].val;
            m_max = m_log[0].val;
            for (const auto &a : m_log) {
                if (m_min > a.val)
                    m_min = a.val;
                if (m_min > a.avg)
                    m_min = a.avg;
                if (m_max < a.val)
                    m_max = a.val;
                if (m_max < a.avg)
                    m_max = a.avg;
            }

            auto m = m_min - (m_min % 300);
            if (m > m_min+5) m -= 300; // +5, чтобы случайные -1-2m не уводили в -300
            m_min = m;
            for (; m <= 15000; m += 300)
                if (m > m_max) break;
            m_max = m;
        }
        void mode(int m, uint16_t cnt) {
            if (m_log.empty()) return;
            if ((m < 0) || (m > 3))
                return;

            auto &a = m_log.back();
            const char s[] = { 'N', 'T', 'F', 'C' };
            a.mode = s[m];
            a.mcnt = cnt;

            if (cnt < m_log.size()) {
                m_log[m_log.size()-1-cnt].ismode = true;
            }
        }

        void btnSmpl(btn_code_t btn) {
            if (btn == BTN_SEL)
                setViewInfoDebug();
        }
        
        void draw(U8G2 &u8g2) {
            // Шрифт для высоты и расстояния
#if HWVER < 4
            u8g2.setFont(u8g2_font_helvB08_tr);
#else // if HWVER < 4
            u8g2.setFont(u8g2_font_fub20_tf);
#endif
            
            // Высота
            ViewMain::drawAlt(u8g2, -1, u8g2.getAscent());

            char s[32];
            u8g2.setFont(menuFont);

            // вертикальная шкала
            u8g2.drawLine(0, 0, 5, 0);
            u8g2.drawLine(0, u8g2.getDisplayHeight()-1, 5, u8g2.getDisplayHeight()-1);
            u8g2.drawLine(0, u8g2.getDisplayHeight()/2, 5, u8g2.getDisplayHeight()/2);

            const auto d = PSTR("%d");
            snprintf_P(s, sizeof(s), d, m_min);
            u8g2.drawTxt(5, u8g2.getDisplayHeight()-1, s);
            snprintf_P(s, sizeof(s), d, m_max);
            u8g2.drawTxt(5, u8g2.getAscent(), s);
            snprintf_P(s, sizeof(s), d, m_min+(m_max-m_min)/2);
            u8g2.drawTxt(5, (u8g2.getDisplayHeight()+u8g2.getAscent()-1)/2, s);

            // рисуем графики
#define LPADD 12
            double dx = static_cast<double>(u8g2.getDisplayWidth()-LPADD) / m_log.capacity();
            double xd = LPADD;
            double dy = static_cast<double>(u8g2.getDisplayHeight()) / (m_max-m_min);
            for (const auto &a : m_log) {
                // точки графика
                int x = static_cast<int>(round(xd));
                int y = static_cast<int>(round(dy * (a.val - m_min)));
                if ((y >= 0) && (y<u8g2.getDisplayHeight())) {
                    y = u8g2.getDisplayHeight()-y-1;
                    u8g2.drawPixel(x, y);
                
                    // высчитанное место изменения режима
                    if (a.ismode)
                        u8g2.drawDisc(x, y, 2);
                }
                
                if (a.mode) {
                    // точка изменения режима (принятия реешения)
                    for (int y = 0; y < u8g2.getDisplayHeight(); y+=3)
                        u8g2.drawPixel(x, y);
                    snprintf_P(s, sizeof(s), PSTR("%c-%d"), a.mode, a.mcnt);
                    u8g2.drawStr(x-u8g2.getStrWidth(s), u8g2.getDisplayHeight()-1, s);
                }

                xd += dx;
            }
#undef LPADD
        }
};
static ViewMainAltChart vAltChart;
void setViewMainAltChart() { viewSet(vAltChart); }

void altchartadd(int val, int avg) { vAltChart.add(val, avg); }
void altchartmode(int m, uint16_t cnt) { vAltChart.mode(m, cnt); }

#endif // FWVER_DEBUG
