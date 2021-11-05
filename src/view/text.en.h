/*
    View: Text lang
*/

#ifndef _view_text_lang_H
#define _view_text_lang_H

#define menuFont        u8g2_font_ncenB08_tr
#define drawTxt         drawStr
#define getTxtWidth     getStrWidth

#define TXT_MENU_EXIT               "> exit"
#define TXT_MENU_HOLD3SEC           "Hold select 3 sec"

#define TXT_MENU_CONFIG_SAVEERROR   "Config save error"

#define TXT_MENU_ON                 "On"
#define TXT_MENU_OFF                "Off"
#define TXT_MENU_YES                "Yes"
#define TXT_MENU_NO                 "No"
#define TXT_MENU_OK                 "OK"
#define TXT_MENU_FAIL               "fail"
#define TXT_MENU_DISABLE            "disable"

#define TXT_MENU_MAINPAGE_NONE      "No change"
#define TXT_MENU_MAINPAGE_LAST      "Last"
#define TXT_MENU_MAINPAGE_GPS       "GPS"
#define TXT_MENU_MAINPAGE_GPSALT    "GPS+Alt"
#define TXT_MENU_MAINPAGE_ALT       "Alti"

#define TXT_MENU_POINT_CURRENT      "Current"
#define TXT_MENU_POINT_NOTUSED      " (no used)"
#define TXT_MENU_POINT_NOCURR       "[no point]"
#define TXT_MENU_POINT_SAVE         "Save position"
#define TXT_MENU_POINT_NOTSELECT    "Point not selected"
#define TXT_MENU_POINT_NOGPS        "GPS not valid"
#define TXT_MENU_POINT_EEPROMFAIL   "EEPROM fail"
#define TXT_MENU_POINT_SAVED        "Point saved"
#define TXT_MENU_POINT_CLEAR        "Clear"
#define TXT_MENU_POINT_CLEARED      "Point cleared"

#define TXT_MENU_DISPLAY_LIGHT      "Light"
#define TXT_MENU_DISPLAY_CONTRAST   "Contrast"

#define TXT_MENU_GND_MANUALSET      "Manual set"
#define TXT_MENU_GND_MANUALNOTALLOW "Manual not allowed"
#define TXT_MENU_GND_CORRECTED      "GND corrected"
#define TXT_MENU_GND_MANUALALLOW    "Allow mnl set"
#define TXT_MENU_GND_AUTOCORRECT    "Auto correct"
#define TXT_MENU_GND_CORRECTONGND   "On GND"

#define TXT_MENU_AUTOMAIN_FF        "Auto FF-screen"
#define TXT_MENU_AUTOMAIN_CNP       "On CNP"
#define TXT_MENU_AUTOMAIN_LAND      "After Land"
#define TXT_MENU_AUTOMAIN_ONGND     "On GND"
#define TXT_MENU_AUTOMAIN_POWERON   "Power On"

#define TXT_MENU_POWEROFF_FORCE     "Force Off"
#define TXT_MENU_POWEROFF_NOFLY     "No Fly hours"
#define TXT_MENU_POWEROFF_AFTON     "Aft. Pwr-On"

#define TXT_MENU_GPSON_CURRENT      "Current State"
#define TXT_MENU_GPSON_POWERON      "Auto-On by Pwr-On"
#define TXT_MENU_GPSON_TRKREC       "Auto-On by Track Rec"
#define TXT_MENU_GPSON_TAKEOFF      "Auto-On by Takeoff Alt"
#define TXT_MENU_GPSON_OFFLAND      "Force-Off by Landing"

#define TXT_MENU_BTNDO_UP           "Btn Up"
#define TXT_MENU_BTNDO_DOWN         "Btn Down"
#define TXT_MENU_BTNDO_NO           "no"
#define TXT_MENU_BTNDO_BACKLIGHT    "BackLight"
#define TXT_MENU_BTNDO_GPSTGL       "GPS On/Off"
#define TXT_MENU_BTNDO_TRKREC       "Track rec"
#define TXT_MENU_BTNDO_POWEROFF     "Power Off"

#define TXT_MENU_TRACK_RECORDING    "Recording"
#define TXT_MENU_TRACK_FORCE        "Force"
#define TXT_MENU_TRACK_AUTO         "Auto"
#define TXT_MENU_TRACK_NOREC        "no"
#define TXT_MENU_TRACK_BYALT        "Auto Rec. by Alt"
#define TXT_MENU_TRACK_COUNT        "Count"
#define TXT_MENU_TRACK_AVAIL        "Avail"
#define TXT_MENU_TRACK_AVAILSEC     "%d s"

#define TXT_MENU_TIME_ZONE          "Zone"

#define TXT_MENU_OPTION_DISPLAY     "Display"
#define TXT_MENU_OPTION_GNDCORRECT  "Gnd Correct"
#define TXT_MENU_OPTION_AUTOSCREEN  "Auto Screen-Mode"
#define TXT_MENU_OPTION_AUTOPOWER   "Auto Power-Off"
#define TXT_MENU_OPTION_AUTOGPS     "Auto GPS-On"
#define TXT_MENU_OPTION_BTNDO       "Button operation"
#define TXT_MENU_OPTION_TRACK       "Track"
#define TXT_MENU_OPTION_TIME        "Time"

#define TXT_MENU_FW_VERSION         "FW version"
#define TXT_MENU_FW_TYPE            "FW type"
#define TXT_MENU_FW_HWREV           "Hardware rev."
#define TXT_MENU_FW_UPDATETO        "Update to:"
#define TXT_MENU_FW_NOUPD           "-no-"

#define TXT_MENU_TEST_CLOCK         "Clock"
#define TXT_MENU_TEST_BATTERY       "Battery"
#define TXT_MENU_TEST_BATTCHARGE    "Batt charge"
#define TXT_MENU_TEST_PRESSURE      "Pressure"
#define TXT_MENU_TEST_PRESSVAL      "(%0.0fkPa) %s"
#define TXT_MENU_TEST_LIGHT         "Light test"
#define TXT_MENU_TEST_GPSDATA       "GPS data"
#define TXT_MENU_TEST_DATADELAY     "(%d ms) %s"
#define TXT_MENU_TEST_NOGPSDATA     "no data"
#define TXT_MENU_TEST_GPSRESTART    "GPS power restart"
#define TXT_MENU_TEST_GPSRESTARTED  "GPS restarted"
#define TXT_MENU_TEST_GPSINIT       "GPS init"
#define TXT_MENU_TEST_GPSINITED     "GPS inited"

#define TXT_MENU_SYSTEM_FACTORYRES  "Factory reset"
#define TXT_MENU_SYSTEM_FORMATROM   "formating eeprom"
#define TXT_MENU_SYSTEM_EEPROMFAIL  "EEPROM fail"
#define TXT_MENU_SYSTEM_CFGRESETED  "Config reseted"
#define TXT_MENU_SYSTEM_FWUPDATE    "FirmWare update"
#define TXT_MENU_SYSTEM_FILES       "Files"
#define TXT_MENU_SYSTEM_GPSSERIAL   "GPS serial"
#define TXT_MENU_SYSTEM_HWTEST      "Hardware test"

#define TXT_MENU_ROOT_GPSPOINT      "GPS points"
#define TXT_MENU_ROOT_JUMPCOUNT     "Jump count"
#define TXT_MENU_ROOT_LOGBOOK       "LogBook"
#define TXT_MENU_ROOT_OPTIONS       "Options"
#define TXT_MENU_ROOT_SYSTEM        "System"
#define TXT_MENU_ROOT_WIFISYNC      "Wifi sync"

#define TXT_MENU_ROOT               "Configuration"

#endif // _view_text_lang_H
