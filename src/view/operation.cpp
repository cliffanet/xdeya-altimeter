

#include "base.h"

#if HWVER >= 5
#include "main.h"

#include "../log.h"
#include "../sys/sys.h"

class ViewOperation : public View {
    WrkProc<WrkOperation> m_wrk;
    const char *m_titlep;

    public:
        ViewOperation(WrkProc<WrkOperation> &wrk, const char *titlep = NULL) :
            m_wrk(wrk),
            m_titlep(titlep)
            {}
        
        void btnSmpl(btn_code_t btn) {
            // короткое нажатие на любую кнопку ускоряет выход,
            // если процес завершился и ожидается таймаут финального сообщения
            if (!m_wrk.isrun())
                setViewMain();
        }
        
        bool useLong(btn_code_t btn) {
            return btn == BTN_SEL;
        }
        void btnLong(btn_code_t btn) {
            // длинное нажатие из любой стадии завершает процесс синхнонизации принудительно
            if (btn != BTN_SEL)
                return;
            m_wrk.stop();
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            u8g2.setFont(menuFont);
                char s[64];
    
            // Заголовок
            if (m_titlep != NULL) {
                u8g2.setDrawColor(1);
                u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
                u8g2.setDrawColor(0);
                strncpy_P(s, m_titlep, sizeof(s));
                s[sizeof(s)-1] = '\0';
                u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
            }
            
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
            
            y += 20;
            if (m_wrk->str()[0] != '\0')
                u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(m_wrk->str()))/2, y, m_wrk->str());

            y += 10;
            if (m_wrk->cmpl().sz > 0) {
                uint8_t p = m_wrk->cmpl().val * 20 / m_wrk->cmpl().sz;
                char *ss = s;
                *ss = '|';
                ss++;
                for (uint8_t i=0; i< 20; i++, ss++)
                    *ss = i < p ? '.' : ' ';
                *ss = '|';
                ss++;
                *ss = '\0';
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            }
        }
};

void viewOperation(WrkProc<WrkOperation> &wrk, const char *titlep) {
    viewOpen<ViewOperation>(wrk, titlep);
}

#endif // HWVER >= 5
