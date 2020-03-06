
#include "../mode.h"
#include "../display.h"
#include "../button.h"

#include "WiFi.h"
#include <vector>

static void wifiExit();


typedef struct {
    char name[33];
    char txt[40];
} wifi_t;
// текущий список пунктов меню для отрисовки и перелистывания
static std::vector<wifi_t> wifiall;
// индекс выбранного пункта меню и самого верхнего видимого на экране
static int16_t isel=0, itop=0;

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayWifi(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    char s[20];
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,0,128,12);
    strcpy_P(s, PSTR("wifi srch"));
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(isel-itop)*10+14,128,10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        uint8_t i = n + itop;    // i - индекс отрисовываемого пункта меню
        if (i >= wifiall.size()) break;     // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        u8g2.setDrawColor(isel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;   // координата y для отрисовки текста (в нескольких местах используется)
        
        const auto &w = wifiall[i];    // пункт меню, который надо сейчас отобразить
        u8g2.drawStr(2, y, w.txt);
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции перехода между пунктами меню
 * ------------------------------------------------------------------------------------------- */
static void wifiUp() {      // Вверх
    isel --;
    if (isel >= 0) {
        if (itop > isel)  // если вылезли вверх за видимую область,
            itop = isel;  // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else { // если упёрлись в самый верх, перескакиваем в конец меню
        isel = wifiall.size()-1;
        itop = wifiall.size() > MENU_STR_COUNT ? wifiall.size()-MENU_STR_COUNT : 0;
    }
}

static void wifiDown() {    // вниз
    isel ++;
    if (isel < wifiall.size()) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((isel > itop) && ((isel-itop) >= MENU_STR_COUNT))
            itop = isel-MENU_STR_COUNT+1;
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        isel = 0;
        itop=0;
    }
}

static void wifiSelect() {    // вниз
    if ((isel < 0) || (isel >= wifiall.size()))
        return;
    
    const auto &w = wifiall[isel];
    WiFi.begin(w.name, "12344321");
}

/* ------------------------------------------------------------------------------------------- *
 *  Вход в меню
 * ------------------------------------------------------------------------------------------- */
void modeWifi() {
    modemain = false;
    displayHnd(displayWifi);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_UP,      BTN_SIMPLE, wifiUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, wifiDown);
    btnHnd(BTN_SEL,     BTN_SIMPLE, wifiSelect);
    btnHnd(BTN_SEL,     BTN_LONG,   wifiExit);
    
    Serial.println(F("mode wifi"));
    
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup done");
    int n = WiFi.scanNetworks();
    Serial.print(F("scan: "));
    Serial.println(n);
    for (int i = 0; i < n; ++i) {
        wifi_t w;
        strncpy(w.name, WiFi.SSID(i).c_str(), sizeof(w.name));
        w.name[sizeof(w.name)-1] = '\0';
        snprintf_P(w.txt, sizeof(w.txt), PSTR("%s (%d) %c"), w.name, WiFi.RSSI(i),
                   (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?' ':'*');
        wifiall.push_back(w);
    }
    
    Serial.println(F("scan end"));
}

/* ------------------------------------------------------------------------------------------- *
 *  Выход из меню
 * ------------------------------------------------------------------------------------------- */
static void wifiExit() {
    isel=0;
    itop=0;
    wifiall.clear();
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    modeMain();     // выходим в главный режим отображения показаний
}
