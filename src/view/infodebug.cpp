
#include "info.h"
#include "main.h"

#include "../power.h" // pwrBattValue()
#include "../clock.h" // sys time
#include "../altimeter.h" // altCalc()
#include "../gps/proc.h"

class ViewInfoDebug : public ViewInfo {
    public:
        ViewInfoDebug() : ViewInfo(27) {}
        
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL) {
                ViewInfo::btnSmpl(btn);
                return;
            }
    
            setViewMain(MODE_MAIN_GPS);
        }
        
        void updStr(uint16_t i) {
            switch (i) {
                case 0:
                    PRNL("Battery RAW");
                    PRNR("%d", pwrBattValue());
                    break;
                case 1:
                    PRNL("Batt voltage");
                    PRNR("%0.2fv", 3.35 * pwrBattValue() / 4095 * 3 / 2);
                    break;
                
                case 2:
                    PRNL("BMP280");
                    PRNR("%.1f Pa", altCalc().presslast());
                    break;
                
                case 3:
                    PRNL("GPS lon");
                    PRNR("%f", GPS_LATLON(gpsInf().lon));
                    break;
                
                case 4:
                    PRNL("GPS lat");
                    PRNR("%f", GPS_LATLON(gpsInf().lat));
                    break;
                
                case 5:
                    PRNL("GPS numSV");
                    PRNR("%u", gpsInf().numSV);
                    break;
                
                case 6:
                    PRNL("GPS gpsFix");
                    PRNR("%u", gpsInf().gpsFix);
                    break;
                
                case 7:
                    PRNL("GPS recv bytes");
                    if (gpsRecv() > 1024*1024)
                        PRNR("%lu M", gpsRecv()/1024/1024);
                    else
                    if (gpsRecv() > 1024)
                        PRNR("%lu k", gpsRecv()/1024);
                    else
                        PRNR("%lu b", gpsRecv());
                    break;
                
                case 8:
                    PRNL("GPS recv error");
                    PRNR("%lu", gpsRecvError());
                    break;
                
                case 9:
                    PRNL("GPS cmd unknown");
                    PRNR("%lu", gpsCmdUnknown());
                    break;
                
                case 10:
                    PRNL("GPS hMSL");
                    PRNR("%.2f m", GPS_MM(gpsInf().hMSL));
                    break;
                
                case 11:
                    PRNL("GPS hAcc");
                    PRNR("%.2f m", GPS_MM(gpsInf().hAcc));
                    break;
                
                case 12:
                    PRNL("GPS vAcc");
                    PRNR("%.2f m", GPS_MM(gpsInf().vAcc));
                    break;
                
                case 13:
                    PRNL("GPS velN");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velN));
                    break;
                
                case 14:
                    PRNL("GPS velE");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velE));
                    break;
                
                case 15:
                    PRNL("GPS velD");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().velD));
                    break;
                
                case 16:
                    PRNL("GPS speed");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().speed));
                    break;
                
                case 17:
                    PRNL("GPS gSpeed");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().gSpeed));
                    break;
                
                case 18:
                    PRNL("GPS heading");
                    PRNR("%.1f deg", GPS_DEG(gpsInf().heading));
                    break;
                
                case 19:
                    PRNL("GPS sAcc");
                    PRNR("%.2f m/s", GPS_CM(gpsInf().sAcc));
                    break;
                
                case 20:
                    PRNL("GPS cAcc");
                    PRNR("%.1f deg", GPS_DEG(gpsInf().cAcc));
                    break;
                
                case 21:
                    PRNL("GPS date");
                    PRNR("%d.%02d.%02d", gpsInf().tm.day, gpsInf().tm.mon, gpsInf().tm.year);
                    break;
                
                case 22:
                    PRNL("GPS time");
                    PRNR("%d:%02d:%02d", gpsInf().tm.h, gpsInf().tm.m, gpsInf().tm.s);
                    break;
                
                case 23:
                    PRNL("GPS nano");
                    PRNR("%.1f", (double)(gpsInf().tm.nano) / 1000);
                    break;
                
                case 24:
                    PRNL("Sys date");
                    PRNR("%d.%02d.%02d", tmNow().day, tmNow().mon, tmNow().year);
                    break;
                
                case 25:
                    PRNL("Sys time");
                    PRNR("%d:%02d:%02d", tmNow().h, tmNow().m, tmNow().s);
                    break;
                
                case 26:
                    PRNL("Sys tm valid");
                    PRNR("%d", tmNow().valid);
                    break;
            }
        }
};
static ViewInfoDebug vInfDebug;
void setViewInfoDebug() { viewSet(vInfDebug); }
