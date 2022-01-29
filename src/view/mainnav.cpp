
#include "main.h"

#include "../gps/compass.h"

/* ------------------------------------------------------------------------------------------- *
 *  Навигация
 * ------------------------------------------------------------------------------------------- */
// PNT - Конвертирование координат X/Y при вращении вокруг точки CX/CY на угол ANG (рад)
#define PNT(x,y,ang,cx,cy)          round(cos(ang) * (x - cx) - sin(ang) * (y - cy)) + cx,  round(sin(ang) * (x - cx) + cos(ang) * (y - cy)) + cy

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
            
            if (compass().ok & 2)
                u8g2.drawLine(w/2-1, h/2-1, w/2-1+(compass().acc.x/0x200), h/2-1-(compass().acc.y/0x200));
            if (compass().ok & 1) {
                u8g2.drawCircle(w/2-1+(compass().mag.x/16), h/2-1-(compass().mag.y/16), 3);
                u8g2.drawCircle(w/2-1, h/2-1, abs(compass().mag.z/16));
            }
            
            if (compass().ok)
                u8g2.drawDisc(w/2-1, h/2-1, 2);
            if ((compass().ok & 3) == 3)
                u8g2.drawDisc(PNT((w/2-1), 2, DEG_TO_RAD*(360-compass().head), (w/2-1), (h/2-1)), 2);
        }
};
static ViewMainNav vNav;
void setViewMainNav() { viewSet(vNav); }
