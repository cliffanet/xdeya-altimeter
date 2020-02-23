/*
    Управление режимами отображения на дисплее
*/

#ifndef _mode_H
#define _mode_H

#include <Arduino.h>

#define MENU_STR_COUNT  5
#define MENU_TIMEOUT    15000

#define MAIN_TIMEOUT    60000

typedef void (*menu_hnd_t)();
typedef void (*menu_val_t)(char *txt);

// Элемент меню
typedef struct {
    char        *name;      // Текстовое название пункта
    menu_hnd_t  enter;      // Обраотка нажатия на среднюю кнопку
    menu_val_t  showval;    // как отображать значение, если требуется
    menu_hnd_t  editup;     // в режиме редактирования меняет значения вверх (функция должна описывать, что и как менять)
    menu_hnd_t  editdown;   // в режиме редактирования меняет значения вниз
    menu_hnd_t  hold;       // Обработка действия при длинном нажатии на среднюю кнопку
} menu_el_t;


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
// Принудительный переход в FF-режим
void modeFF(); 

// Режим меню настроек
void modeMenu();

#endif // _mode_H
