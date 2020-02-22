/*
    Display
*/

#ifndef _display_H
#define _display_H

#include <Arduino.h>
#include <U8g2lib.h>

// Пин подсветки дисплея
#define LIGHT_PIN 32

/* Прорисовка на дисплее */
typedef void (*display_hnd_t)(U8G2 &u8g2);

void displayInit();
void displayHnd(display_hnd_t hnd);
void displayClear();
void displayUpdate();

/* Управление дисплеем */
void displayOn();
void displayOff();
void displayLightTgl();
bool displayLight();
void displayContrast(uint8_t value);

#endif // _display_H
