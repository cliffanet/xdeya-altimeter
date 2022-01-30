/*
    View: Text lang
*/

#ifndef _view_text_lang_H
#define _view_text_lang_H

#include "../../def.h"
#include <stdint.h>
#include <clib/u8g2.h>
extern const uint8_t u8g2_font_robotobd_08_tx[] U8G2_FONT_SECTION("u8g2_font_robotobd_08_tx");

//#define menuFont        u8g2_font_6x12_t_cyrillic
#define menuFont        u8g2_font_robotobd_08_tx
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
#define TXT_MENU_MAINPAGE_ALTNAV    "Нав+Высот"
#define TXT_MENU_MAINPAGE_ALT       "Высотомер"
#define TXT_MENU_MAINPAGE_NAV       "Навигация"

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
#define TXT_MENU_DISPLAY_FLIPP180   "Развернуть на 180"

#define TXT_MENU_GND_MANUALSET      "Сбросить вручную"
#define TXT_MENU_GND_MANUALNOTALLOW "Ручной сброс запрещён"
#define TXT_MENU_GND_CORRECTED      "Уровень земли установлен"
#define TXT_MENU_GND_MANUALALLOW    "Разрешение ручн. сбр."
#define TXT_MENU_GND_AUTOCORRECT    "Автоподстройка"
#define TXT_MENU_GND_CORRECTONGND   "На земле"
#define TXT_MENU_GND_ALTCORRECT     "Превышение площадки"

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
#define TXT_MENU_GPSON_OFFLAND      "Выключать при приземлении"

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

#define TXT_MENU_SYSTEM_RESTART     "Перезагрузка"
#define TXT_MENU_SYSTEM_FACTORYRES  "Сброс всех настроек"
#define TXT_MENU_SYSTEM_FORMATROM   "Форматирование"
#define TXT_MENU_SYSTEM_EEPROMFAIL  "Ошибка EEPROM"
#define TXT_MENU_SYSTEM_CFGRESETED  "Настройки сброшены"
#define TXT_MENU_SYSTEM_FWUPDATE    "Обновление прошивки"
#define TXT_MENU_SYSTEM_FILES       "Файлы"
#define TXT_MENU_SYSTEM_GPSSERIAL   "GPS serial"
#define TXT_MENU_SYSTEM_GPSSATINFO  "GPS спутники"
#define TXT_MENU_SYSTEM_HWTEST      "Тестирование аппарат."

#define TXT_MENU_ROOT_GPSPOINT      "GPS точки"
#define TXT_MENU_ROOT_JUMPCOUNT     "Кол-во прыжков"
#define TXT_MENU_ROOT_LOGBOOK       "LogBook"
#define TXT_MENU_ROOT_OPTIONS       "Параметры"
#define TXT_MENU_ROOT_SYSTEM        "Система"
#define TXT_MENU_ROOT_WIFISYNC      "Wifi синхронизация"

#define TXT_MENU_ROOT               "Настройки"

#define TXT_SERVER_DNSFAIL          "ошибка адреса сервера"
#define TXT_SERVER_LOST             "потеря связи с сервером"
#define TXT_SERVER_READFAIL         "ошибка при получении данных"
#define TXT_SERVER_PROTOFAIL        "ошибка протокола передачи"
#define TXT_SERVER_SENDFAIL         "ошибка при передаче данных"
#define TXT_SERVER_SENDTIMEOUT      "зависла отправка данных"

#define TXT_WIFI_CONNECTTONET       "Подключение к %s"
#define TXT_WIFI_WEBSYNC            "Web Sync"
#define TXT_WIFI_NET                "WiFi"
#define TXT_WIFI_STATUS_OFF         "WiFi откл."
#define TXT_WIFI_STATUS_DISCONNECT  "WiFi не подкл."
#define TXT_WIFI_STATUS_FAIL        "Ошибка WiFi"
#define TXT_WIFI_WAITJOIN           "Ожидаем привязки"
#define TXT_WIFI_WAIT_SEC           "%d сек"
#define TXT_WIFI_RSSI               "%3d dBm"
#define TXT_WIFI_SEARCHNET          "... поиск сетей ..."
#define TXT_WIFI_NEEDPASSWORD       "Требуется пароль!"

#define TXT_WIFI_MSG_SENDCONFIG     "Отправка настроек..."
#define TXT_WIFI_MSG_SENDJUMPCOUNT  "Отправка кол-ва прыжков..."
#define TXT_WIFI_MSG_SENDPOINT      "Отправка точек..."
#define TXT_WIFI_MSG_SENDLOGBOOK    "Отправка прыжков..."
#define TXT_WIFI_MSG_SENDTRACK      "Отправка треков..."
#define TXT_WIFI_MSG_SENDFIN        "Завершение отправки..."
#define TXT_WIFI_MSG_JOINACCEPT     "Привязка к Web завершена"

#define TXT_WIFI_NEXT_HELLO         "Ожидание ответа сервера"
#define TXT_WIFI_NEXT_CONFIRM       "Ожидание подтверждения..."
#define TXT_WIFI_NEXT_RCVPASS       "Приём паролей WiFi"
#define TXT_WIFI_NEXT_RCVFWVER      "Приём версий прошивки"
#define TXT_WIFI_NEXT_FWUPDATE      "Обновление прошивки"
#define TXT_WIFI_NEXT_FIN           "Ожидание завершения..."

#define TXT_WIFI_ERR_WIFIINIT       "Ошибка инициализации WiFi"
#define TXT_WIFI_ERR_WIFICONNECT    "Ошибка подключения WiFi"
#define TXT_WIFI_ERR_WIFITIMEOUT    "Ошибка ожидания wifi"
#define TXT_WIFI_ERR_TIMEOUT        "Ошибка ожидания (таймаут)"
#define TXT_WIFI_ERR_JOINLOAD       "Ошибка чтения Web-привязки"
#define TXT_WIFI_ERR_SENDAUTH       "Ошибка авторизации"
#define TXT_WIFI_ERR_SENDCONFIG     "Ошибка отправки настроек"
#define TXT_WIFI_ERR_SENDJUMPCOUNT  "Ошибка отправки кол-ва прыжков"
#define TXT_WIFI_ERR_SENDPOINT      "Ошибка отправки точек"
#define TXT_WIFI_ERR_SENDLOGBOOK    "Ошибка отправки прыжков"
#define TXT_WIFI_ERR_SENDTRACK      "Ошибка отправки треков"
#define TXT_WIFI_ERR_SENDFIN        "Ошибка завершения отправки"
#define TXT_WIFI_ERR_SERVERCONNECT  "Ошибка подключения к серверу"
#define TXT_WIFI_ERR_RCVCMDUNKNOWN  "Получена неизвестная команда"
#define TXT_WIFI_ERR_SAVEJOIN       "Ошибка сохранеиня Web-привязки"
#define TXT_WIFI_ERR_SENDJOIN       "Ошибка отправки Web-привязки"
#define TXT_WIFI_ERR_PASSCLEAR      "Ошибка сброса WiFi-паролей"
#define TXT_WIFI_ERR_VERAVAILCLEAR  "Ошибка сброса версий прошивки"
#define TXT_WIFI_ERR_WIFIADD        "Ошибка добавления WiFi-паролей"
#define TXT_WIFI_ERR_VERAVAIL       "Ошибка добавления версий прош."
#define TXT_WIFI_ERR_FWSIZEBIG      "Слишком большой размер прошивки"
#define TXT_WIFI_ERR_FWINIT         "Ошибка инициализации прошивки"
#define TXT_WIFI_ERR_FWWRITE        "Ошибка установки прошивки"
#define TXT_WIFI_ERR_FWFIN          "Ошибка завершения прошивки"

#define TXT_WIFI_SYNCFINISHED       "Синхронизация успешно завершена"

#define TXT_LOGBOOK_JUMPNUM         "Прыжок # %d"
#define TXT_LOGBOOK_ALTEXIT         "Отделение"
#define TXT_LOGBOOK_ALTCNP          "Раскрытие"
#define TXT_LOGBOOK_TIMETOFF        "Длит. взлёта"
#define TXT_LOGBOOK_TIMEFF          "Длит. своб.п."
#define TXT_LOGBOOK_TIMESEC         "%d с"
#define TXT_LOGBOOK_TIMECNP         "Длит. пилотир"

#define TXT_MAIN_VERTSPEED          "%.0f км/ч"
#define TXT_MAIN_GPSSTATE_OFF       "gps откл"
#define TXT_MAIN_GPSSTATE_INIT      "gps иниц"
#define TXT_MAIN_GPSSTATE_INITFAIL  "gps ош.иниц."
#define TXT_MAIN_GPSSTATE_NODATA    "нет gps данных"
#define TXT_MAIN_GPSSTATE_SATCOUNT  "сп: %d"

#define TXT_MAIN_VSPEED_MS          "%0.1f м/с"
#define TXT_MAIN_3DSPEED_MS         "%0.1f м/с"

#endif // _view_text_lang_H
