
#include "info.h"
#include "main.h"

#include "../power.h" // pwrBattValue()
#include "../altimeter.h" // altCalc()
#include "../gps.h"

class ViewInfoDebug : public ViewInfo {
    public:
        ViewInfoDebug() : ViewInfo(21) {}
        
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
                    PRNL("BMP280");
                    PRNR("%.1f Pa", altCalc().presslast());
                    break;
                
                case 2:
                    PRNL("GPS longitude");
                    PRNR("%d", gpsInf().lon);
                    break;
                
                case 3:
                    PRNL("GPS latitude");
                    PRNR("%d", gpsInf().lat);
                    break;
                
                case 4:
                    PRNL("GPS numSV");
                    PRNR("%u", gpsInf().numSV);
                    break;
                
                case 5:
                    PRNL("GPS gpsFix");
                    PRNR("%u", gpsInf().gpsFix);
                    break;
                
                case 6:
                    PRNL("GPS recv error");
                    PRNR("%lu", gpsRecvError());
                    break;
                
                case 7:
                    PRNL("GPS cmd unknown");
                    PRNR("%lu", gpsCmdUnknown());
                    break;
                
                case 8:
                    PRNL("GPS hMSL");
                    PRNR("%ld mm", gpsInf().hMSL);
                    break;
                
                case 9:
                    PRNL("GPS hAcc");
                    PRNR("%lu mm", gpsInf().hAcc);
                    break;
                
                case 10:
                    PRNL("GPS vAcc");
                    PRNR("%lu mm", gpsInf().vAcc);
                    break;
                
                case 11:
                    PRNL("GPS velN");
                    PRNR("%ld cm/s", gpsInf().velN);
                    break;
                
                case 12:
                    PRNL("GPS velE");
                    PRNR("%ld cm/s", gpsInf().velE);
                    break;
                
                case 13:
                    PRNL("GPS velD");
                    PRNR("%ld cm/s", gpsInf().velD);
                    break;
                
                case 14:
                    PRNL("GPS speed");
                    PRNR("%ld cm/s", gpsInf().speed);
                    break;
                
                case 15:
                    PRNL("GPS gSpeed");
                    PRNR("%ld cm/s", gpsInf().gSpeed);
                    break;
                
                case 16:
                    PRNL("GPS heading");
                    PRNR("%ld deg", gpsInf().heading);
                    break;
                
                case 17:
                    PRNL("GPS sAcc");
                    PRNR("%lu cm/s", gpsInf().sAcc);
                    break;
                
                case 18:
                    PRNL("GPS cAcc");
                    PRNR("%lu deg", gpsInf().cAcc);
                    break;
                
                case 19:
                    PRNL("GPS date");
                    PRNR("%d.%02d.%02d", gpsInf().tm.day, gpsInf().tm.mon, gpsInf().tm.year);
                    break;
                
                case 20:
                    PRNL("GPS time");
                    PRNR("%d:%02d:%02d", gpsInf().tm.h, gpsInf().tm.m, gpsInf().tm.s);
                    break;
            }
        }
};
static ViewInfoDebug vInfDebug;
void setViewInfoDebug() { viewSet(vInfDebug); }
