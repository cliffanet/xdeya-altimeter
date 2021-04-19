
#include "menu.h"
#include "main.h"

// заголовок меню
static char title[MENUSZ_NAME] = { '\0' };
static menu_dspl_el_t str[MENU_STR_COUNT];

// редактируется ли текущий пункт меню
static uint8_t valflash = 0; // В режиме редактирования помогает мигать отображению значения
// Сообщение моргающим текстом - будет выводится на селекторе меню (в этот момент сам выделенный пункт меню скрывается)
static char flashstr[MENUSZ_NAME] = { '\0' };
static int16_t flashcnt = 0; // счётчик мигания - это и таймер мигания, и помогает мигать

/* ------------------------------------------------------------------------------------------- *
 *  инициализация меню
 * ------------------------------------------------------------------------------------------- */
ViewMenu::ViewMenu(uint16_t _sz, menu_exit_t _exit) :
    sz(_sz),
    elexit(_exit)
{
    if (_exit != MENUEXIT_NONE)
        sz++;
}

void ViewMenu::setSize(uint16_t _sz) {
    sz = _sz;
    
    if (elexit != MENUEXIT_NONE)
        sz++;
    
    if (isel >= sz)
        isel = sz-1;
}

void ViewMenu::open(ViewMenu *_mprev, const char *_title) {
    mprev = _mprev;
    titlep = _title;
    itop = 0;
    isel = 0;
    restore();
}

void ViewMenu::restore() {
    if (titlep == NULL)
        title[0] = '\0';
    else {
        strncpy_P(title, titlep, sizeof(title));
        title[sizeof(title)-1]='\0';
    }
        
    updStr();
}


/* ------------------------------------------------------------------------------------------- *
 *  обновление списка видимых строк
 * ------------------------------------------------------------------------------------------- */
void ViewMenu::updStr() {
    for (int16_t i=itop, n=0; n<MENU_STR_COUNT; i++, n++)
        if (i<sz)
            updStr(i);
        else {
            str[n].name[0] = '\0';
            str[n].val[0] = '\0';
        }
    
    updValFlash();
}

void ViewMenu::updStr(int16_t i) {
    int16_t n = i-itop;
    if ((n < 0) || (n >= MENU_STR_COUNT))
        return;
    
    if (isExit(i)) {
        strcpy_P(str[n].name, PSTR("Exit"));
        str[n].val[0] = '\0';
    }
    else {
        if (elexit == MENUEXIT_TOP)
            i--;
        auto &s = str[n];
        getStr(s, i);
        s.name[sizeof(s.name)-1] = '\0';
        s.val[sizeof(s.val)-1] = '\0';
    }
    Serial.printf("ViewMenu::updStr: [%d] %d: %s\r\n", i, n, str[n].name);
}
void ViewMenu::setTop(int16_t _itop) {
    itop = _itop;
    updStr();
}

void ViewMenu::setSel(int16_t i) {
    if (i < 0)
        i = 0;
    if (i >= sz)
        i = sz-1;
    isel = i;
    
    if (itop > isel)  // если вылезли вверх за видимую область,
        setTop(isel); // спускаем список ниже, чтобы отобразился выделенный пункт
    // если вылезли вниз за видимую область,
    // поднимаем список выше, чтобы отобразился выделенный пункт
    if ((isel > itop) && ((isel-itop) >= MENU_STR_COUNT))
        setTop(isel-MENU_STR_COUNT+1);
}

void ViewMenu::updValFlash() {
    valflash = valFlash() ? 1 : 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  обработчики кнопок
 * ------------------------------------------------------------------------------------------- */
void ViewMenu::btnSmpl(btn_code_t btn) {
    switch (btn) {
        case BTN_UP:
            isel --;
            if (isel >= 0) {
                if (itop > isel)  // если вылезли вверх за видимую область,
                    setTop(isel); // спускаем список ниже, чтобы отобразился выделенный пункт
            }
            else { // если упёрлись в самый верх, перескакиваем в конец меню
                isel = sz-1;
                if (sz > MENU_STR_COUNT)
                    setTop(sz-MENU_STR_COUNT);
            }
    
            Serial.printf("selUp: %d (%d) => %s / %s\r\n", isel, sz, str[isel-itop].name, str[isel-itop].val);
            break;
            
        case BTN_DOWN:
            isel ++;
            if (isel < sz) {
                // если вылезли вниз за видимую область,
                // поднимаем список выше, чтобы отобразился выделенный пункт
                if ((isel > itop) && ((isel-itop) >= MENU_STR_COUNT))
                    setTop(isel-MENU_STR_COUNT+1);
            }
            else { // если упёрлись в самый низ, перескакиваем в начало меню
                isel = 0;
                if (sz > MENU_STR_COUNT)
                    setTop(0);
            }
    
            Serial.printf("selDn: %d (%d) => %s / %s\r\n", isel, sz, str[isel-itop].name, str[isel-itop].val);
            break;
            
        case BTN_SEL:
            // выход в предыдущее меню
            if (mprev == NULL)
                setViewMain();
            else {
                viewSet(*mprev);
                mprev->restore();
            }
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Прорисовка меню на дисплее
 * ------------------------------------------------------------------------------------------- */
void ViewMenu::draw(U8G2 &u8g2) {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    u8g2.drawBox(0,0,128,12);
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(title))/2, 10, title);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(isel-itop)*10+14,128,10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STR_COUNT; n++) {
        uint8_t i = n + itop;   // i - индекс отрисовываемого пункта меню
        if (i >= sz) break;     // для меню, которые полностью отрисовались и осталось ещё место, не выходим за список меню
        u8g2.setDrawColor(isel == i ? 0 : 1); // Выделенный пункт рисуем прозрачным, остальные обычным
        int8_t y = (n+1)*10-1+14;   // координата y для отрисовки текста (в нескольких местах используется)
        
        if ((flashcnt > 0) && (isel == i)) {
            // если нужно вывести моргающее сообщение, делаем это вместо отрисовки
            // текстовой части этого пункта меню.
            // Внимание зрителя в этот момент как раз сосредоточено на этом пункте,
            // ему не надо будет бегать глазами по экрану в ругое место,
            // а текстовая часть самого меню в момент отображения мигающего сообщения как раз не нужна.
            if ((flashcnt--) % 8 <= 4) {
                // алгоритм мигания. При отрисовке каждые 100мс полный цикла потухания и зажигания равен 0.8 сек
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(flashstr))/2, y, flashstr);
            }
            continue;
        }
        
        const auto &m = str[n];
        u8g2.drawStr(2, y, m.name);
        if ((m.val[0] != '\0') &&      // вывод значения
            ((valflash == 0) || (i != isel) || ((valflash++) % 8 <= 4))) // мерцание редактируемого значения
            // выводим значение, если есть обработчик
            // в режиме редактирования - мигаем, и только выбранным пунктом меню
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(m.val)-2, y, m.val);
    }
    u8g2.setDrawColor(1);
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции мигания сообщением на выбранном пункте меню
 * ------------------------------------------------------------------------------------------- */
void menuFlashP(char *txt, int16_t count) {  // Запоминаем текст сообщения и сколько тактов отображения показывать
    strncpy_P(flashstr, txt, sizeof(flashstr));
    flashstr[sizeof(flashstr)] = '\0';
    flashcnt=count;
}

void menuFlashHold() {       // Стандартное сообщение о том, что для данной процедуры нужно удерживать
                                // среднюю кнопку три секунды
    menuFlashP(PSTR("Hold select 3 sec"), 30);
}

void menuFlashClear() {      // Очистка моргающего сообщения в случае продолжения пользования меню
    flashstr[0] = '\0';
    flashcnt=0;
}
