/*
    Display
*/

#ifndef _display_H
#define _display_H

#include "../def.h"

typedef void (*display_hnd_t)(U8G2 &u8g2);

void displayInit();
void displayHnd(display_hnd_t hnd);
void displayUpdate();

#endif // _display_H
