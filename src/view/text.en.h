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
#define TXT_MENU_DISPLAY_FLIPP180   "Flipped 180"

#define TXT_MENU_GND_MANUALSET      "Manual set"
#define TXT_MENU_GND_MANUALNOTALLOW "Manual not allowed"
#define TXT_MENU_GND_CORRECTED      "GND corrected"
#define TXT_MENU_GND_MANUALALLOW    "Allow mnl set"
#define TXT_MENU_GND_AUTOCORRECT    "Auto correct"
#define TXT_MENU_GND_CORRECTONGND   "On GND"
#define TXT_MENU_GND_ALTCORRECT     "Alt correction"

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

#define TXT_SERVER_DNSFAIL          "DNS lookup failed"
#define TXT_SERVER_LOST             "server connect lost"
#define TXT_SERVER_READFAIL         "fail read from server"
#define TXT_SERVER_PROTOFAIL        "recv proto fail"
#define TXT_SERVER_SENDFAIL         "server send fail"
#define TXT_SERVER_SENDTIMEOUT      "server send timeout"

#define TXT_WIFI_CONNECTTONET       "wifi to %s"
#define TXT_WIFI_WEBSYNC            "Web Sync"
#define TXT_WIFI_NET                "WiFi"
#define TXT_WIFI_STATUS_OFF         "WiFi off"
#define TXT_WIFI_STATUS_DISCONNECT  "Disconnected"
#define TXT_WIFI_STATUS_FAIL        "Connect fail"
#define TXT_WIFI_WAITJOIN           "Wait to JOIN"
#define TXT_WIFI_WAIT_SEC           "%d sec"
#define TXT_WIFI_RSSI               "%3d dBm"
#define TXT_WIFI_SEARCHNET          "... searching ..."
#define TXT_WIFI_NEEDPASSWORD       "Password required!"

#define TXT_WIFI_MSG_SENDCONFIG     "Sending config..."
#define TXT_WIFI_MSG_SENDJUMPCOUNT  "Sending jump count..."
#define TXT_WIFI_MSG_SENDPOINT      "Sending point..."
#define TXT_WIFI_MSG_SENDLOGBOOK    "Sending LogBook..."
#define TXT_WIFI_MSG_SENDTRACK      "Sending Tracks..."
#define TXT_WIFI_MSG_SENDFIN        "Sending fin..."
#define TXT_WIFI_MSG_JOINACCEPT     "join accepted"

#define TXT_WIFI_NEXT_HELLO         "wait hello"
#define TXT_WIFI_NEXT_CONFIRM       "Wait server confirm..."
#define TXT_WIFI_NEXT_RCVPASS       "Recv wifi pass"
#define TXT_WIFI_NEXT_RCVFWVER      "Recv FW-versions"
#define TXT_WIFI_NEXT_FWUPDATE      "FW-update"
#define TXT_WIFI_NEXT_FIN           "Wait server fin..."

#define TXT_WIFI_ERR_WIFIINIT       "WiFi init fail"
#define TXT_WIFI_ERR_WIFICONNECT    "WiFi connect fail"
#define TXT_WIFI_ERR_WIFITIMEOUT    "wifi timeout"
#define TXT_WIFI_ERR_TIMEOUT        "timeout"
#define TXT_WIFI_ERR_JOINLOAD       "Can\'t load JOIN-inf"
#define TXT_WIFI_ERR_SENDAUTH       "auth send fail"
#define TXT_WIFI_ERR_SENDCONFIG     "send cfg fail"
#define TXT_WIFI_ERR_SENDJUMPCOUNT  "send cmp count fail"
#define TXT_WIFI_ERR_SENDPOINT      "send pnt fail"
#define TXT_WIFI_ERR_SENDLOGBOOK    "send LogBook fail"
#define TXT_WIFI_ERR_SENDTRACK      "send Tracks fail"
#define TXT_WIFI_ERR_SENDFIN        "Finishing send fail"
#define TXT_WIFI_ERR_SERVERCONNECT  "server can't connect"
#define TXT_WIFI_ERR_RCVCMDUNKNOWN  "recv unknown cmd"
#define TXT_WIFI_ERR_SAVEJOIN       "save fail"
#define TXT_WIFI_ERR_SENDJOIN       "join send fail"
#define TXT_WIFI_ERR_PASSCLEAR      "WiFi clear Fail"
#define TXT_WIFI_ERR_VERAVAILCLEAR  "Versions clear Fail"
#define TXT_WIFI_ERR_WIFIADD        "WiFi add Fail"
#define TXT_WIFI_ERR_VERAVAIL       "FW-version add Fail"
#define TXT_WIFI_ERR_FWSIZEBIG      "FW-size too big"
#define TXT_WIFI_ERR_FWINIT         "FW-init fail"
#define TXT_WIFI_ERR_FWWRITE        "FW-write fail"
#define TXT_WIFI_ERR_FWFIN          "FW-finish fail"

#define TXT_WIFI_SYNCFINISHED       "Sync finished"

#define TXT_LOGBOOK_JUMPNUM         "Jump # %d"
#define TXT_LOGBOOK_ALTEXIT         "Alt"
#define TXT_LOGBOOK_ALTCNP          "Deploy"
#define TXT_LOGBOOK_TIMETOFF        "Takeoff"
#define TXT_LOGBOOK_TIMEFF          "FF time"
#define TXT_LOGBOOK_TIMESEC         "%d s"
#define TXT_LOGBOOK_TIMECNP         "CNP time"

#define TXT_MAIN_VERTSPEED          "%.0f km/h"
#define TXT_MAIN_GPSSTATE_OFF       "gps off"
#define TXT_MAIN_GPSSTATE_INIT      "gps init"
#define TXT_MAIN_GPSSTATE_INITFAIL  "gps init fail"
#define TXT_MAIN_GPSSTATE_NODATA    "no gps data"
#define TXT_MAIN_GPSSTATE_SATCOUNT  "s: %d"

#define TXT_MAIN_VSPEED_MS          "%0.1f m/s"
#define TXT_MAIN_3DSPEED_MS         "%0.1f m/s"

#endif // _view_text_lang_H
