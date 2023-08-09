
#include "main.h"
#include "info.h"
#include "../core/worker.h"
#include "../core/filetxt.h"
#include "../jump/proc.h"
#include "../filtlib/ring.h"
#include "../clock.h"
#include "../monlog.h"

#ifdef FWVER_DEBUG

    typedef struct {
        uint16_t interval;
        int val;
        int avg;
        float speed;
        float sqdiff;
        char mode;
        bool ismode;
        uint16_t mcnt;
    } alt_t;

class altChartWrk : public Wrk {
    ring<alt_t> m_log;
    FileTxt m_fh;
    uint32_t m_i = 0;

public:
    altChartWrk(ring<alt_t> &_log) : m_log(_log) {
        CONSOLE("save altchart(0x%08x) begin: %d", this, m_log.size());
    }
    ~altChartWrk() {
        CONSOLE("save altchart(0x%08x) destroy", this);
    }
    
    state_t run() {

    WPROC
#ifdef USE_SDCARD
        if (!fileExtInit())
            return END;
#else
        return END;
#endif
        
    WPRC_RUN
    char fname[64];
    auto tm = tmNow();
    snprintf_P(
        fname, sizeof(fname),
        PSTR("/xdeya_altlog-%04d-%02d-%02d_%02d%02d%02d.csv"),
        tm.year, tm.mon, tm.day,
        tm.h, tm.m, tm.s
    );
    if (!m_fh.open(fname, FileMy::MODE_WRITE, true)) {
        CONSOLE("open fail");
        return END;
    }

    WPRC_RUN
    char head[256];
    strncpy_P(head, PSTR("interval;alt;avg;speed;sqdiff;ismode;mode"), sizeof(head));
    head[sizeof(head)-1] = '\0';
    if (!m_fh.print_line(head)) {
        CONSOLE("white head fail");
        return END;
    }

    WPRC_RUN
    const auto &a = m_log[m_i];
    char m[32];
    if (a.mode)
        snprintf_P(m, sizeof(m), PSTR("%c-%d"), a.mode, a.mcnt);
    else
        m[0] = '\0';
    char sp[16];
    snprintf_P(sp, sizeof(sp), PSTR("%0.1f"), a.speed);
    sp[strlen(sp)-2] = ',';
    char sq[16];
    snprintf_P(sq, sizeof(sq), PSTR("%0.1f"), a.sqdiff);
    sq[strlen(sq)-2] = ',';
    char s[256];
    snprintf_P(
        s, sizeof(s), PSTR("%u;%d;%d;%s;%s;%d;%s"),
        a.interval, a.val, a.avg, sp, sq, a.ismode, m
    );
    if (!m_fh.print_line(s)) {
        CONSOLE("white(%u) fail", m_i);
        return END;
    }

    m_i++;
    if (m_i < m_log.size())
        return RUN;
        
    WPRC(END)
    }
    
    void end() {
        m_fh.close();
        fileExtStop();
        CONSOLE("save altchart(0x%08x) stopped", this);
    }
};

class ViewMainAltChart : public ViewMain {
    ring<alt_t> m_log;
    int m_min = 0, m_max = 0;
    int m_save = 0;

    public:
        ViewMainAltChart() : m_log(60*10*3) { }

        void add(uint16_t interval, int val, int avg, float speed, float sqdiff) {
            m_log.push_back({ interval, val, avg, speed, sqdiff, '\0', false, 0 });

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
            switch (btn) {
                case BTN_SEL:
                    setViewInfoDebug();
                    break;
                case BTN_UP:
                    m_save = 80;
                    break;
                case BTN_DOWN:
                    m_save = 305;
                    break;
            }
        }
        
        void draw(U8G2 &u8g2) {
            // Шрифт для высоты и расстояния
#if HWVER < 4
            u8g2.setFont(u8g2_font_helvB08_tr);
#else // if HWVER < 4
            u8g2.setFont(u8g2_font_fub20_tf);
#endif
            
            // Высота
            int y = u8g2.getAscent();
            ViewMain::drawAlt(u8g2, -1, y);

            char s[32];
            u8g2.setFont(menuFont);

            // параметры altcalc
            y += u8g2.getAscent() + 5;
            snprintf_P(s, sizeof(s), PSTR("sq: %0.1f (%ds)"), altCalc().sqdiff(), altCalc().sqbigtm()/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
            y += u8g2.getAscent() + 2;
            snprintf_P(s, sizeof(s), PSTR("state: %d (%ds)"), altCalc().state(), altCalc().statetm()/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
            y += u8g2.getAscent() + 2;
            snprintf_P(s, sizeof(s), PSTR("jmp: %d (%ds)"), altCalc().jmpmode(), altCalc().jmptm()/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
            y += u8g2.getAscent() + 2;
            snprintf_P(s, sizeof(s), PSTR("ff: %d (%d) (%ds)"), altCalc().ffprof(), altCalc().ffalt(), altCalc().ffproftm()/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);

            if (m_save > 0) {
                // сообщение о сохранении в файл
                if (m_save > 5)
                    snprintf_P(s, sizeof(s), PSTR("save %d sec"), (m_save-5)/5);
                else
                    snprintf_P(s, sizeof(s), PSTR("saving..."));
                u8g2.drawTxt(30, u8g2.getAscent(), s);

                if (m_save == 5)
                    wrkRun<altChartWrk>(m_log);
                
                m_save--;
            }

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

void altchartadd(uint16_t interval, int val, int avg, float speed, float sqdiff) {
    vAltChart.add(interval, val, avg, speed, sqdiff);
}
void altchartmode(int m, uint16_t cnt) { vAltChart.mode(m, cnt); }

#endif // FWVER_DEBUG
