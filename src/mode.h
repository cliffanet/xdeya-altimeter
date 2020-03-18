/*
    Управление режимами отображения на дисплее
*/

#ifndef _mode_H
#define _mode_H

#include <Arduino.h>

#define MAIN_TIMEOUT    60000

// Вырианты отображения основного режима
#define MODE_MAIN_MIN       1
#define MODE_MAIN_MAX       4
// Использовать текущий экран (не менять)
#define MODE_MAIN_NONE      0
// Использовать крайний запомненный (до каких либо автоматических изменений)
#define MODE_MAIN_LAST      -1
// Экран отображения только показаний GPS
#define MODE_MAIN_GPS       1
// Экран отображения высоты (большими цифрами)
#define MODE_MAIN_ALT       2
// Экран отображения высоты и GPS
#define MODE_MAIN_ALTGPS    3
// Экран отображения времени
#define MODE_MAIN_TIME      4

extern bool modemain;

// Основной режим - отображается одна из страниц с показаниями
void modeMain();
void initMain(int8_t m);
// Принудительный переход в FF-режим
void modeFF();

// Режим меню настроек
void modeMenu();

// Режим выбора wifi-сети
void modeWifi();

// Режим выбора LogBook
void modeLogBook(size_t i = 0);

#endif // _mode_H
