/*
    View: Logo
*/

#ifndef _view_logo_H
#define _view_logo_H

#include "base.h"

// Вырианты отображения основного режима (в количестве итераций - для view - это 200ms на итерацию)
#define LOGO_TIME           20

class ViewLogo : public View {
    public:
        void draw(U8G2 &u8g2);
        void process();
    private:
        uint8_t waitn = 0;
};

void setViewLogo();

#endif // _view_logo_H
