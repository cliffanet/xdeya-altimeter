/*
    Управление режимами отображения на дисплее
*/

#ifndef _mode_H
#define _mode_H

#include <Arduino.h>

#define MENU_STR_COUNT  5
#define MENU_TIMEOUT    15000

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

// Основной режим - отображается одна из страниц с показаниями
void modeMain();

// Режим меню настроек
void modeMenu();

#endif // _mode_H
