
#include "info.h"
#include "main.h"

#include "../power.h" // pwrBattValue()
#include "../clock.h" // sys time
#include "../jump/proc.h" // altCalc()
#include "../gps/proc.h"

class ViewInfoDebug : public ViewInfo {
    public:
        ViewInfoDebug() : ViewInfo(37) {}
        
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL) {
                ViewInfo::btnSmpl(btn);
                return;
            }
    
            setViewMain(MODE_MAIN_GPS);
        }
        
        void updStr(uint16_t i) {
            switch (i) {
#if HWVER > 1
                case 0:
                    PRNL("Battery RAW");
                    PRNR("%d", pwrBattValue());
                    break;
                case 1:
                    PRNL("Batt voltage");
                    PRNR("%0.2fv", 3.35 * pwrBattValue() / 4095 * 3 / 2);
                    break;
#endif
                
                case 2:
                    PRNL("BMP280");
                    PRNR("%.3f kPa", altCalc().press()/1000);
                    break;
                
                case 3:
                    PRNL("Press GND");
                    PRNR("%.3f kPa", altCalc().pressgnd()/1000);
                    break;
                
                case 4:
                    PRNL("Alt");
                    PRNR("%.1f m", altCalc().alt());
                    break;
                
                case 5:
                    PRNL("Alt avg");
                    PRNR("%.1f m", altCalc().altavg());
                    break;
                
                case 6:
                    PRNL("Alt appr");
                    PRNR("%.1f m", altCalc().altapp());
                    break;
                
                case 7:
                    PRNL("VSpeed avg");
                    PRNR("%.1f m/s", altCalc().speedavg());
                    break;
                
                case 8:
                    PRNL("VSpeed appr");
                    PRNR("%.1f m/s", altCalc().speedapp());
                    break;
                
                case 9:
                    PRNL("app kb");
                    PRNR("%.3f", altCalc().kb());
                    break;
                
                case 10:
                    PRNL("sqdiff");
                    PRNR("%.3f", altCalc().sqdiff());
                    break;
                
                case 11:
                    PRNL("Alt dir");
                    switch (altCalc().direct()) {
                        case ACDIR_INIT:    PRNR("init / %d s", altCalc().dirtm()/1000);    break;
                        case ACDIR_ERR:     PRNR("err / %d s",  altCalc().dirtm()/1000);    break;
                        case ACDIR_UP:      PRNR("up / %d s",   altCalc().dirtm()/1000);    break;
                        case ACDIR_FLAT:    PRNR("flat / %d s", altCalc().dirtm()/1000);    break;
                        case ACDIR_DOWN:    PRNR("down / %d s", altCalc().dirtm()/1000);    break;
                    }
                    break;
                
                case 12:
                    PRNL("Alt mode");
                    switch (altCalc().state()) {
                        case ACST_INIT:     PRNR("init / %d s", altCalc().statetm()/1000);  break;
                        case ACST_GROUND:   PRNR("gnd / %d s",  altCalc().statetm()/1000);  break;
                        case ACST_TAKEOFF40:PRNR("to40 / %d s", altCalc().statetm()/1000);  break;
                        case ACST_TAKEOFF:  PRNR("toff / %d s", altCalc().statetm()/1000);  break;
                        case ACST_FREEFALL: PRNR("ff / %d s",   altCalc().statetm()/1000);  break;
                        case ACST_CANOPY:   PRNR("cnp / %d s",  altCalc().statetm()/1000);  break;
                        case ACST_LANDING:  PRNR("land / %d s", altCalc().statetm()/1000);  break;
                    }
                    break;
                
                case 13:
                    PRNL("Jmp state");
                    switch (jmpState()) {
                        case 0: PRNR("-");      break;
                        case 1: PRNR("FF");     break;
                        case 2: PRNR("CNP");    break;
                    }
                    break;
                
                case 14:
                    PRNL("GPS lon");
                    PRNR("%f", GPS_LATLON(gpsInf().lon));
                    break;
                
                case 15:
                    PRNL("GPS lat");
                    PRNR("%f", GPS_LATLON(gpsInf().lat));
                    break;
                
                case 16:
                    PRNL("GPS numSV");
                    PRNR("%u", gpsInf().numSV);
                    break;
                
                case 17:
                    PRNL("GPS gpsFix");
                    PRNR("%u", gpsInf().gpsFix);
                    break;
                
                case 18:
                    PRNL("GPS recv bytes");
                    if (gpsRecv() > 1024*1024)
                        PRNR("%lu M", gpsRecv()/1024/1024);
                    else
                    if (gpsRecv() > 1024)
                        PRNR("%lu k", gpsRecv()/1024);
                    else
                        PRNR("%lu b", gpsRecv());
                    break;
                
                case 19:
                    PRNL("GPS recv error");
                    PRNR("%lu", gpsRecvError());
                    break;
                
                case 20:
                    PRNL("GPS cmd unknown");
                    PRNR("%lu", gpsCmdUnknown());
                    break;
                
                case 21:
                    PRNL("GPS hMSL");
                    PRNR("%.2f m", GPS_MM(gpsInf().hMSL));
                    break;
                
                case 22:
                    PRNL("GPS hAcc");
                    PRNR("%.2f m", GPS_MM(gpsInf().hAcc));
                    break;
                
                case 23:
                    PRNL("GPS vAcc");
                    PRNR("%.2f m", GPS_MM(gpsInf().vAcc));
                    break;
                
                case 24:
                    PRNL("GPS velN");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velN));
                    break;
                
                case 25:
                    PRNL("GPS velE");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velE));
                    break;
                
                case 26:
                    PRNL("GPS velD");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velD));
                    break;
                
                case 27:
                    PRNL("GPS speed");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().speed));
                    break;
                
                case 28:
                    PRNL("GPS gSpeed");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().gSpeed));
                    break;
                
                case 29:
                    PRNL("GPS heading");
                    PRNR("%.1f deg", GPS_DEG(gpsInf().heading));
                    break;
                
                case 30:
                    PRNL("GPS sAcc");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().sAcc));
                    break;
                
                case 31:
                    PRNL("GPS cAcc");
                    PRNR("%.1f deg", GPS_DEG(gpsInf().cAcc));
                    break;
                
                case 32:
                    PRNL("GPS date");
                    PRNR("%d.%02d.%02d", gpsInf().tm.day, gpsInf().tm.mon, gpsInf().tm.year);
                    break;
                
                case 33:
                    PRNL("GPS time");
                    PRNR("%d:%02d:%02d.%02d", gpsInf().tm.h, gpsInf().tm.m, gpsInf().tm.s, gpsInf().tm.cs);
                    break;
                
                case 34:
                    PRNL("Sys date");
                    PRNR("%d.%02d.%02d", tmNow().day, tmNow().mon, tmNow().year);
                    break;
                
                case 35:
                    PRNL("Sys time");
                    PRNR("%d:%02d:%02d.%02d", tmNow().h, tmNow().m, tmNow().s, tmNow().cs);
                    break;
                
                case 36:
                    PRNL("Sys tm valid");
                    PRNR("%d", tmValid() ? 1 : 0);
                    break;
            }
        }
};
static ViewInfoDebug vInfDebug;
void setViewInfoDebug() { viewSet(vInfDebug); }
