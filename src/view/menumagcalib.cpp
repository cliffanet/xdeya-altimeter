
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../navi/compass.h"

/* ------------------------------------------------------------------------------------------- *
 *  ViewMagCalib - процесс калибровки магнетометра
 * ------------------------------------------------------------------------------------------- */

class ViewMagCalib : public ViewBase {
    public:
        ViewMagCalib() :
            ViewBase()
        {
            init();
        }
        
        void init() {
            m_cur = compass().mag;
            m_min = m_cur;
            m_max = m_cur;
            saved = false;
        }
        
        // фоновый процессинг - обрабатываем этапы синхронизации
        void process() {
            if (saved)
                return;
            
            m_cur = compass().mag;
            if (m_min.x > m_cur.x)
                m_min.x = m_cur.x;
            if (m_min.y > m_cur.y)
                m_min.y = m_cur.y;
            if (m_min.z > m_cur.z)
                m_min.z = m_cur.z;
            if (m_max.x < m_cur.x)
                m_max.x = m_cur.x;
            if (m_max.y < m_cur.y)
                m_max.y = m_cur.y;
            if (m_max.z < m_cur.z)
                m_max.z = m_cur.z;
        }
        
        bool useLong(btn_code_t btn) {
            return true;
        }
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            setViewMain();
        }
        void btnLong(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            if (saved)
                return;
            compCalibrate(m_min, m_max);
            saved =  true;
        }
        
        void drawVec(U8G2 &u8g2, int x, const vec16_t &v) {
            int y = 15;
            char s[20];
            const char *d = PSTR("%d");
            
            sprintf_P(s, d, v.x);
            u8g2.drawStr(x-u8g2.getStrWidth(s), y, s);
            y += 10;
            
            sprintf_P(s, d, v.y);
            u8g2.drawStr(x-u8g2.getStrWidth(s), y, s);
            y += 10;
            
            sprintf_P(s, d, v.z);
            u8g2.drawStr(x-u8g2.getStrWidth(s), y, s);
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            u8g2.setDrawColor(1);
            u8g2.setFont(menuFont);
            
            int y = 15, xc = u8g2.getDisplayWidth()/2, x1 = xc/2;
            char s[80];
            strcpy_P(s, PSTR("x ="));
            u8g2.drawStr(xc - 25, y, s);
            y += 10;
            strcpy_P(s, PSTR("y ="));
            u8g2.drawStr(xc - 25, y, s);
            y += 10;
            strcpy_P(s, PSTR("z ="));
            u8g2.drawStr(xc - 25, y, s);
            
            drawVec(u8g2, xc-45, m_min);
            drawVec(u8g2, xc+25, m_cur);
            drawVec(u8g2, xc+62, m_max);
            
            y += 20;
            strcpy_P(s, saved ? PTXT(MCAL_SAVED) : PTXT(MCAL_SAVE));
            u8g2.drawTxt(0, y, s);
        }
        
    private:
        vec16_t m_min, m_cur, m_max;
        bool saved;
};
ViewMagCalib vMagCalib;

void setViewMagCalib() {
    vMagCalib.init();
    viewSet(vMagCalib);
}
