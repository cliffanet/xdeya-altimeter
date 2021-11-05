/*
    View: Text lang
*/

#ifndef _view_text_lang_H
#define _view_text_lang_H

#define menuFont        u8g2_font_6x12_t_cyrillic
#define drawTxt         drawUTF8
#define getTxtWidth     getUTF8Width

#define TXT_MENU_EXIT               "> выход"
#define TXT_MENU_HOLD3SEC           "Удерж кнопку 3 сек"

#define TXT_MENU_CONFIG_SAVEERROR   "Ошибка сохранения"

#define TXT_MENU_ON                 "вкл"
#define TXT_MENU_OFF                "выкл"
#define TXT_MENU_YES                "да"
#define TXT_MENU_NO                 "нет"
#define TXT_MENU_OK                 "OK"
#define TXT_MENU_FAIL               "ошибка"
#define TXT_MENU_DISABLE            "выкл"

#define TXT_MENU_MAINPAGE_NONE      "не менять"
#define TXT_MENU_MAINPAGE_LAST      "предыдущая"
#define TXT_MENU_MAINPAGE_GPS       "GPS"
#define TXT_MENU_MAINPAGE_GPSALT    "GPS+Высот"
#define TXT_MENU_MAINPAGE_ALT       "Высотомер"

#define TXT_MENU_POINT_CURRENT      "Выбранная точка"
#define TXT_MENU_POINT_NOTUSED      " (нет)"
#define TXT_MENU_POINT_NOCURR       "[без точки]"
#define TXT_MENU_POINT_SAVE         "Сохранить коорд."
#define TXT_MENU_POINT_NOTSELECT    "Точка не выбрана"
#define TXT_MENU_POINT_NOGPS        "Нет GPS спутников"
#define TXT_MENU_POINT_EEPROMFAIL   "Ошибка сохранения"
#define TXT_MENU_POINT_SAVED        "Точка сохранена"
#define TXT_MENU_POINT_CLEAR        "Очистить точку"
#define TXT_MENU_POINT_CLEARED      "Точка очищена"

#define TXT_MENU_DISPLAY_LIGHT      "Подсветка"
#define TXT_MENU_DISPLAY_CONTRAST   "Контраст"

#define TXT_MENU_GND_MANUALSET      "Сбросить вручную"
#define TXT_MENU_GND_MANUALNOTALLOW "Ручной сброс запрещён"
#define TXT_MENU_GND_CORRECTED      "Уровень земли установлен"
#define TXT_MENU_GND_MANUALALLOW    "Разрешение ручн. сбр."
#define TXT_MENU_GND_AUTOCORRECT    "Автоподстройка"
#define TXT_MENU_GND_CORRECTONGND   "На земле"

#define TXT_MENU_AUTOMAIN_FF        "Своб. падение"
#define TXT_MENU_AUTOMAIN_CNP       "После раскрытия"
#define TXT_MENU_AUTOMAIN_LAND      "При приземлении"
#define TXT_MENU_AUTOMAIN_ONGND     "Долго на земле"
#define TXT_MENU_AUTOMAIN_POWERON   "При включении"

#define TXT_MENU_POWEROFF_FORCE     "Выключить вручную"
#define TXT_MENU_POWEROFF_NOFLY     "Без взлётов"
#define TXT_MENU_POWEROFF_AFTON     "После включения"

#define TXT_MENU_GPSON_CURRENT      "Текущее состояние"
#define TXT_MENU_GPSON_POWERON      "Вкл. при включении"
#define TXT_MENU_GPSON_TRKREC       "Вкл. при записи трека"
#define TXT_MENU_GPSON_TAKEOFF      "Вкл. на высоте"
#define TXT_MENU_GPSON_OFFLAND      "Вычключать при приземлении"

#define TXT_MENU_BTNDO_UP           "Кнопка Вверх"
#define TXT_MENU_BTNDO_DOWN         "Кнопка Вниз"
#define TXT_MENU_BTNDO_NO           "не исп."
#define TXT_MENU_BTNDO_BACKLIGHT    "Подсветка"
#define TXT_MENU_BTNDO_GPSTGL       "GPS вкл/выкл"
#define TXT_MENU_BTNDO_TRKREC       "Запись трека"
#define TXT_MENU_BTNDO_POWEROFF     "Выкл. питание"

#define TXT_MENU_TRACK_RECORDING    "Писать сейчас"
#define TXT_MENU_TRACK_FORCE        "Вручную"
#define TXT_MENU_TRACK_AUTO         "Автоматически"
#define TXT_MENU_TRACK_NOREC        "выкл"
#define TXT_MENU_TRACK_BYALT        "Автозапись на высоте"
#define TXT_MENU_TRACK_COUNT        "Кол-во треков"
#define TXT_MENU_TRACK_AVAIL        "Доступно для записи"
#define TXT_MENU_TRACK_AVAILSEC     "%d с"

#define TXT_MENU_TIME_ZONE          "Час. пояс"

#define TXT_MENU_OPTION_DISPLAY     "Дисплей"
#define TXT_MENU_OPTION_GNDCORRECT  "Подст. уровня земли"
#define TXT_MENU_OPTION_AUTOSCREEN  "Автоперекл. экрана"
#define TXT_MENU_OPTION_AUTOPOWER   "Автовыключение"
#define TXT_MENU_OPTION_AUTOGPS     "Управление режимом GPS"
#define TXT_MENU_OPTION_BTNDO       "Функции кнопок"
#define TXT_MENU_OPTION_TRACK       "Трэк"
#define TXT_MENU_OPTION_TIME        "Часы"

#define TXT_MENU_FW_VERSION         "Версия прошивки"
#define TXT_MENU_FW_TYPE            "Тип прошивки"
#define TXT_MENU_FW_HWREV           "Аппарат. вер."
#define TXT_MENU_FW_UPDATETO        "Обновить до:"
#define TXT_MENU_FW_NOUPD           "-нет-"

#define TXT_MENU_TEST_CLOCK         "Часы"
#define TXT_MENU_TEST_BATTERY       "Батарея"
#define TXT_MENU_TEST_BATTCHARGE    "Подкл. зарядка"
#define TXT_MENU_TEST_PRESSURE      "Давление"
#define TXT_MENU_TEST_PRESSVAL      "(%0.0fкПа) %s"
#define TXT_MENU_TEST_LIGHT         "Подсветка"
#define TXT_MENU_TEST_GPSDATA       "GPS данные"
#define TXT_MENU_TEST_DATADELAY     "(%d мс) %s"
#define TXT_MENU_TEST_NOGPSDATA     "нет данных"
#define TXT_MENU_TEST_GPSRESTART    "Перезапустить GPS"
#define TXT_MENU_TEST_GPSRESTARTED  "GPS перезапущен"
#define TXT_MENU_TEST_GPSINIT       "Инициализир. GPS"
#define TXT_MENU_TEST_GPSINITED     "GPS переинициализирован"

#define TXT_MENU_SYSTEM_FACTORYRES  "Сброс всех настроек"
#define TXT_MENU_SYSTEM_FORMATROM   "Форматирование"
#define TXT_MENU_SYSTEM_EEPROMFAIL  "Ошибка EEPROM"
#define TXT_MENU_SYSTEM_CFGRESETED  "Настройки сброшены"
#define TXT_MENU_SYSTEM_FWUPDATE    "Обновление прошивки"
#define TXT_MENU_SYSTEM_FILES       "Файлы"
#define TXT_MENU_SYSTEM_GPSSERIAL   "GPS serial"
#define TXT_MENU_SYSTEM_HWTEST      "Тестирование аппарат."

#define TXT_MENU_ROOT_GPSPOINT      "GPS точки"
#define TXT_MENU_ROOT_JUMPCOUNT     "Кол-во прыжков"
#define TXT_MENU_ROOT_LOGBOOK       "LogBook"
#define TXT_MENU_ROOT_OPTIONS       "Параметры"
#define TXT_MENU_ROOT_SYSTEM        "Система"
#define TXT_MENU_ROOT_WIFISYNC      "Wifi синхронизация"

#define TXT_MENU_ROOT               "Настройки"

#endif // _view_text_lang_H
