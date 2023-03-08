
#include "menu.h"
#include "main.h"
#include "../log.h"

#include <vector>

// редактируется ли текущий пункт меню
// Сообщение моргающим текстом - будет выводится на селекторе меню (в этот момент сам выделенный пункт меню скрывается)
static char flashstr[MENUSZ_NAME] = { '\0' };
static int16_t flashcnt = 0; // счётчик мигания - это и таймер мигания, и помогает мигать

/* ------------------------------------------------------------------------------------------- *
 *  инициализация меню
 * ------------------------------------------------------------------------------------------- */
ViewMenu::ViewMenu(uint16_t _sz, exit_t _exit) :
    sz(_sz),
    elexit(_exit)
{
    if (_exit != EXIT_NONE)
        sz++;
}

void ViewMenu::setSize(uint16_t _sz) {
    sz = _sz;
    
    if (elexit != EXIT_NONE)
        sz++;
    
    if (isel >= sz)
        isel = sz-1;
}

void ViewMenu::setTitle(const char *_title) {
    titlep = _title;
}

void ViewMenu::setTop(int16_t _itop) {
    itop = _itop;
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
            break;
            
        case BTN_SEL:
            // выход в предыдущее меню
            menuPrev();
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Прорисовка меню на дисплее
 * ------------------------------------------------------------------------------------------- */
void ViewMenu::draw(U8G2 &u8g2) {
    u8g2.setFont(menuFont);
    
    // Заголовок
    char title[MENUSZ_NAME];
    if (titlep == NULL)
        title[0] = '\0';
    else {
        strncpy_P(title, titlep, sizeof(title));
        title[sizeof(title)-1]='\0';
    }
    u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
    u8g2.setDrawColor(0);
    u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(title))/2, 10, title);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(isel-itop)*10+14,u8g2.getDisplayWidth(),10);
    
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
            if ((flashcnt--) % 8 <= 4)
                // алгоритм мигания. При отрисовке каждые 100мс полный цикла потухания и зажигания равен 0.8 сек
                u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(flashstr))/2, y, flashstr);
            continue;
        }
        
        // Получение текстовых данных текущей строки
        // Раньше мы эти строчки кешировали в массиве, обновляя
        // этот массив каждый раз при смещении меню, но
        // в итоге от этого кеширования отказались, т.к. часто
        // данные требуют оперативного обновления, а затраты на
        // их получение минимальны, при этом очень много гемора,
        // связанного с обновлением кеша
        line_t m;
        if (isExit(i)) {
            strcpy_P(m.name, PTXT(MENU_EXIT));
            m.val[0] = '\0';
        }
        else {
            getStr(m, elexit == EXIT_TOP ? i-1 : i);
            m.name[sizeof(m.name)-1] = '\0';
            m.val[sizeof(m.val)-1] = '\0';
        }

        u8g2.drawTxt(2, y, m.name);
        if ((m.val[0] != '\0') &&      // вывод значения
            ((i != isel) || !valFlash() || isblink())) // мерцание редактируемого значения
            // выводим значение, если есть обработчик
            // в режиме редактирования - мигаем, и только выбранным пунктом меню
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(m.val)-2, y, m.val);
    }
    u8g2.setDrawColor(1);
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции мигания сообщением на выбранном пункте меню
 * ------------------------------------------------------------------------------------------- */
static std::vector<ViewMenu *> menuall;
void _menuAdd(ViewMenu *menu) {
    viewSet(*menu);
    menu->setTitle(menuall.size() > 0 ? menuall.back()->getSelName() : PTXT(MENU_ROOT));
    // Перерисуем, чтобы особо медленные в инициализации,
    // подвешивая процессор, всё же показывали, что переключение
    // всё-таки произошло (кнопка нажалась).
    displayUpdate();
    menu->restore();
    menuall.push_back(menu);
    CONSOLE("size: %d", menuall.size());
}
ViewMenu *menuTop() {
    return
        menuall.size() > 0 ?
            menuall.back() :
            NULL;
}
ViewMenu *menuPrev() {
    // выход в предыдущее меню
    if (menuall.size() == 0)
        return NULL;
    
    auto *m = menuall.back();
    menuall.pop_back();
    CONSOLE("size: %d", menuall.size());
    delete m;
    
    if (menuall.size() > 0) {
        auto prev = menuall.back();
        viewSet(*prev);
        displayUpdate();
        prev->restore();
        return prev;
    }
    else
        setViewMain();
    
    return NULL;
}
ViewMenu *menuRestore() {
    // возобновление верхнего меню,
    // актуально, когда мы делаем viewSet без menuClear
    // и надо вернуть обратно меню
    if (menuall.size() == 0)
        return NULL;
    
    auto prev = menuall.back();
    viewSet(*prev);
    displayUpdate();
    prev->restore();
    return prev;
}
void menuClear() {
    if (menuall.size() == 0)
        return;
    
    for (auto m: menuall)
        delete m;
    
    menuall.clear();
    setViewMain();
    CONSOLE("cleared");
}

void menuFlashP(const char *txt, int16_t count) {  // Запоминаем текст сообщения и сколько тактов отображения показывать
    strncpy_P(flashstr, txt, sizeof(flashstr));
    flashstr[sizeof(flashstr)] = '\0';
    flashcnt=count;
}

void menuFlashHold() {       // Стандартное сообщение о том, что для данной процедуры нужно удерживать
                                // среднюю кнопку три секунды
    menuFlashP(PTXT(MENU_HOLD3SEC), 30);
}

void menuFlashClear() {      // Очистка моргающего сообщения в случае продолжения пользования меню
    flashstr[0] = '\0';
    flashcnt=0;
}
