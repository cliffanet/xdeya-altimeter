/*
    Управление режимами отображения на дисплее
*/

#ifndef _mode_H
#define _mode_H

#include <Arduino.h>

#define MAIN_TIMEOUT    60000

extern void (*loopMain)();

extern bool modemain;
extern void (*hndProcess)();

// Переход в основной режим - отображается одна из страниц с показаниями
void modeMain();
// Изменение текущего экрана в modeMain без включения этого режима, например для cfg apply
void setModeMain(int8_t m);

// Режим меню настроек
void modeMenu();

// Режим выбора wifi-сети
void modeWifi();

// Режим синхронизации с сервером
void modeNetSync(const char *_ssid, const char *_pass = NULL);

// Режим выбора LogBook
void modeLogBook(size_t i = 0);

#endif // _mode_H
