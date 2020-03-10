
#include "base.h"
#include "../display.h"
#include "../button.h"
#include "../mode.h"

#include <vector>

static std::vector<MenuBase *> mtree;

static char title[20] = { '\0' };
static menu_dspl_el_t str[MENU_STRCOUNT];

static button_hnd_t editup = NULL, editdn = NULL, editenter = NULL;
// редактируется ли текущий пункт меню
static bool menuedit = false;
// Сообщение моргающим текстом - будет выводится на селекторе меню (в этот момент сам выделенный пункт меню скрывается)
static char flashstr[20] = { '\0' };
static int16_t flashcnt = 0; // счётчик мигания - это и таймер мигания, и помогает мигать

static void menuEditOn();
static void menuLevelUp();
static void menuExit();

MenuBase::MenuBase(const char *_title, uint16_t _sz, menu_exit_t _exit) :
    sz(_sz),
    elexit(_exit)
{
    strncpy_P(title, _title, sizeof(title));
    title[sizeof(title)-1]='\0';
    if (_exit != MENUEXIT_NONE)
        sz++;
}


/* ------------------------------------------------------------------------------------------- *
 *  Инициализация меню - обновляем содержимое в строковом варианте
 * ------------------------------------------------------------------------------------------- */
void MenuBase::updStr() {
    for (int16_t i=itop, n=0; (i<sz) && (n<MENU_STRCOUNT); i++, n++)
        updStr(i);
}

void MenuBase::updStr(int16_t i) {
    int16_t n = i-itop;
    if ((n < 0) || (n >= MENU_STRCOUNT))
        return;
    
    if (isExit(i)) {
        strcpy_P(str[n].name, PSTR("Exit"));
        str[n].val[0] = '\0';
    }
    else {
        if (elexit == MENUEXIT_TOP)
            i--;
        updStr(str[n], i);
    }
    Serial.printf("MenuBase::updStr: [%d] %d: %s\r\n", i, n, str[n].name);
}

/* ------------------------------------------------------------------------------------------- *
 *  Обновление обработчиков кнопок при каждом нажатии
 * ------------------------------------------------------------------------------------------- */
void MenuBase::updHnd() {
    button_hnd_t smp = NULL, lng = menuExit;
    
    editup = NULL;
    editdn = NULL;
    
    if (((elexit == MENUEXIT_TOP) && (isel == 0)) ||
        ((elexit == MENUEXIT_BOTTOM) && ((isel+1) == sz))) {
        smp = menuLevelUp;
        
    }
    else {
        updHnd(elexit == MENUEXIT_TOP ? isel-1 : isel, smp, lng, editup, editdn);
    }
    
    if ((editup != NULL) && (editdn != NULL)) {
        btnHnd(BTN_SEL, BTN_SIMPLE, menuEditOn);
        editenter = smp;
    }
    else {
        btnHnd(BTN_SEL, BTN_SIMPLE, smp);
        editenter = NULL;
    }
    
    btnHnd(BTN_SEL, BTN_LONG, lng);
}

/* ------------------------------------------------------------------------------------------- *
 *  Прорисовка меню на дисплее
 * ------------------------------------------------------------------------------------------- */
void MenuBase::displayDraw(U8G2 &u8g2) {
    //char s[20]; // строка для вытаскивания строк из PROGMEM
    static uint8_t valflash = 0; // В режиме редактирования помогает мигать отображению значения
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    u8g2.drawBox(0,0,128,12);
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(title))/2, 10, title);
    
    // выделение пункта меню, текст будем писать поверх
    u8g2.setDrawColor(1);
    u8g2.drawBox(0,(isel-itop)*10+14,128,10);
    
    // Выводим видимую часть меню, n - это индекс строки на экране
    for (uint8_t n = 0; n<MENU_STRCOUNT; n++) {
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
            if ((flashcnt--) % 4 >= 2) {
                // алгоритм мигания. При отрисовке каждые 200мс полный цикла потухания и зажигания равен 0.8 сек
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(flashstr))/2, y, flashstr);
            }
            continue;
        }
        
        const auto &m = str[n];
        u8g2.drawStr(2, y, m.name);
        if ((m.val[0] != '\0') &&      // вывод значения
            (!menuedit || (i != isel) || ((valflash++) % 4 >= 2))) // мерцание редактируемого значения
            // выводим значение, если есть обработчик
            // в режиме редактирования - мигаем, и только выбранным пунктом меню
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(m.val)-2, y, m.val);
    }
    u8g2.setDrawColor(1);
    
}

/* ------------------------------------------------------------------------------------------- *
 *  Перемещение курсора по пунктам меню
 * ------------------------------------------------------------------------------------------- */
void MenuBase::setTop(int16_t _itop) {
    itop = _itop;
    updStr();
}
void MenuBase::selUp() {
    isel --;
    if (isel >= 0) {
        if (itop > isel)  // если вылезли вверх за видимую область,
            setTop(isel); // спускаем список ниже, чтобы отобразился выделенный пункт
    }
    else { // если упёрлись в самый верх, перескакиваем в конец меню
        isel = sz-1;
        if (sz > MENU_STRCOUNT)
            setTop(sz-MENU_STRCOUNT);
    }
    
    Serial.printf("selUp: %d (%d)\r\n", isel, sz);
    
    updHnd();
}

void MenuBase::selDn() {
    isel ++;
    if (isel < sz) {
        // если вылезли вниз за видимую область,
        // поднимаем список выше, чтобы отобразился выделенный пункт
        if ((isel > itop) && ((isel-itop) >= MENU_STRCOUNT))
            setTop(isel-MENU_STRCOUNT+1);
    }
    else { // если упёрлись в самый низ, перескакиваем в начало меню
        isel = 0;
        if (sz > MENU_STRCOUNT)
            setTop(0);
    }
    
    Serial.printf("selDn: %d (%d)\r\n", isel, sz);
    
    updHnd();
}

/* ------------------------------------------------------------------------------------------- *
 *  Выход из меню
 * ------------------------------------------------------------------------------------------- */
void MenuBase::levelUp()  { menuLevelUp(); }
void MenuBase::exit()     { menuExit(); }

/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки меню
 * ------------------------------------------------------------------------------------------- */
static void displayMenu(U8G2 &u8g2) {
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->displayDraw(u8g2);
}

/* ------------------------------------------------------------------------------------------- *
 *  Нажатия кнопок
 * ------------------------------------------------------------------------------------------- */
static void menuUp() {
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->selUp();
}
static void menuDown() {
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->selDn();
}

/* ------------------------------------------------------------------------------------------- *
 *  Функции режима редактирования
 * ------------------------------------------------------------------------------------------- */
static void editUp() {      // Вверх
    if (editup == NULL) return;
    editup();
    menuFlashClear();
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->updStrSel();
}

static void editDown() {    // вниз
    if (editdn == NULL) return;
    editdn();
    menuFlashClear();
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->updStrSel();
}

static void menuEditOff() {     // Выход из режима редактирования
    menuedit = false;   // возвращаем обратно определения кнопок
    btnHnd(BTN_UP,      BTN_SIMPLE, menuUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, menuDown);
    
    if (mtree.size() == 0) return;
    auto m = mtree.back();
    m->updHnd();
}

static void menuEditOn() {      // Вход в режим редактирования
    if ((editup == NULL) && (editdn == NULL))
        // доп контроль, что это редактируемое поле
        return;
    
    // Переназначаем кнопки
    btnHnd(BTN_UP,      BTN_SIMPLE, editUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, editDown);
    btnHnd(BTN_SEL,     BTN_SIMPLE, menuEditOff);
    
    if (editenter != NULL)    // При входе в режим редактирования можно использовать обработчик enter (как и при обычном нажатии)
       editenter();
    
    menuedit = true;
    menuFlashClear();
}

/* ------------------------------------------------------------------------------------------- *
 *  Выход из меню - на уровень выше и совсем
 * ------------------------------------------------------------------------------------------- */
static void menuLevelUp() {
    auto m = mtree.back();
    mtree.pop_back();
    delete m;
    menuFlashClear();
    if (mtree.empty())
        menuExit();
}

static void menuExit() {
    for (auto m : mtree)
        delete m;
    mtree.clear();
    menuFlashClear();
    modeMain();
}

/* ------------------------------------------------------------------------------------------- *
 *  Вход в меню
 * ------------------------------------------------------------------------------------------- */
void menuEnter(MenuBase *menu) {
    displayHnd(displayMenu);    // обработчик отрисовки на экране
    btnHndClear();              // Назначаем обработчики кнопок (средняя кнопка назначается в menuSubMain() через menuHnd())
    btnHnd(BTN_UP,      BTN_SIMPLE, menuUp);
    btnHnd(BTN_DOWN,    BTN_SIMPLE, menuDown);
    btnHnd(BTN_SEL, BTN_LONG,   [] () {
            auto m = mtree.back();
            mtree.pop_back();
            delete m;
            if (mtree.empty())
                modeMain();
        });
    
    Serial.println(F("mode menu: "));
    menu->updStr();
    menu->updHnd();
    mtree.push_back(menu);
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
    menuFlashP(PSTR("Hold select 3 sec"), 15);
}

void menuFlashClear() {      // Очистка моргающего сообщения в случае продолжения пользования меню
    flashstr[0] = '\0';
    flashcnt=0;
}
