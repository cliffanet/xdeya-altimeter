
#include "main.h"
#include "../filtlib/ring.h"
#include "../clock.h"
#include "../monlog.h"

#ifdef FWVER_DEBUG

class ViewMainLog : public ViewMain {
    typedef struct {
        uint16_t h;
        uint8_t m;
        uint8_t s;
        uint16_t ms;
        char str[58];
    } log_t;
    ring<log_t> m_log;

    public:
        ViewMainLog() : m_log(20) { monlog_P(PSTR("log init")); }

        void vlog(const char *s, va_list ap) {
            uint64_t ms = utm() / 1000;
            log_t l;

            l.h = ms / (3600*1000);
            ms -= l.h* (3600*1000);
            l.m = ms / (60*1000);
            ms -= l.m* (60*1000);
            l.s = ms / 1000;
            ms -= l.s* 1000;
            l.ms = ms;

            vsnprintf(l.str, sizeof(l.str), s, ap);
            m_log.push_back(l);
        }

        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            
            setViewMain(MODE_MAIN_ALTNAV);


        }
        
        void draw(U8G2 &u8g2) {
            u8g2.setFont(u8g2_font_squeezed_r6_tr);
            int16_t y = -1 * m_log.size() * (u8g2.getAscent()+1) + u8g2.getDisplayHeight();
            if (y > 0) y = 0;
            for (const auto &l : m_log) {
                y += u8g2.getAscent()+1;
                if (y <= 0) continue;

                char t[32];
                snprintf_P(t, sizeof(t), PSTR("%2u:%02u:%02u.%03u"), l.h, l.m, l.s, l.ms);
                u8g2.drawStr(0, y, t);
                u8g2.drawStr(45, y, l.str);
            }
        }
};
static ViewMainLog vLog;
void setViewMainLog() { viewSet(vLog); }

void monlog_P(const char *s, ...) {
    va_list ap;
    char str[strlen_P(s)+1];

    strcpy_P(str, s);
    
    va_start (ap, s);
    vLog.vlog(str, ap);
    va_end (ap);
}

#endif // FWVER_DEBUG
