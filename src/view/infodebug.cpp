
#include "info.h"
#include "main.h"

#include "../power.h" // pwrBattRaw()
#include "../clock.h" // sys time
#include "../jump/proc.h" // altCalc()
#include "../navi/proc.h"
#include "../navi/compass.h"

#ifdef FWVER_DEBUG

class ViewInfoDebug : public ViewInfo {
    public:
        ViewInfoDebug() : ViewInfo(55) {}
        
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL) {
                ViewInfo::btnSmpl(btn);
                return;
            }
    
            setViewMain(MODE_MAIN_ALTNAV);
        }
        
        void updStr(uint16_t i) {
            switch (i) {
#if HWVER > 1
                case 0:
                    PRNL("Battery RAW");
                    PRNR("%d", pwrBattRaw());
                    break;
                case 1:
                    PRNL("Batt voltage");
                    PRNR("%0.2fv", pwrBattValue());
                    break;
#endif
                
                case 2:
                    PRNL("Pwr mode");
                    switch (pwrMode()) {
                        case PWR_OFF:       PRNR("off");        break;
                        case PWR_SLEEP:     PRNR("sleep");      break;
                        case PWR_PASSIVE:   PRNR("passive");    break;
                        case PWR_ACTIVE:    PRNR("active");     break;
                    }
                    break;
                
                case 3:
                    PRNL("btnIdle");
                    PRNR("%0.1f s", ((float)btnIdle())/1000);
                    break;
                
                case 4:
                    PRNL("BMP280");
                    PRNR("%.3f kPa", altCalc().press()/1000);
                    break;
                
                case 5:
                    PRNL("Press GND");
                    PRNR("%.3f kPa", altCalc().pressgnd()/1000);
                    break;
                
                case 6:
                    PRNL("Alt");
                    PRNR("%.1f m", altCalc().alt());
                    break;
                
                case 7:
                    PRNL("Alt avg");
                    PRNR("%.1f m", altCalc().altavg());
                    break;
                
                case 8:
                    PRNL("Alt appr");
                    PRNR("%.1f m", altCalc().altapp());
                    break;
                
                case 9:
                    PRNL("VSpeed avg");
                    PRNR("%.1f m/s", altCalc().speedavg());
                    break;
                
                case 10:
                    PRNL("VSpeed appr");
                    PRNR("%.1f m/s", altCalc().speedapp());
                    break;
                
                case 11:
                    PRNL("app kb");
                    PRNR("%.3f", altCalc().kb());
                    break;
                
                case 12:
                    PRNL("sqdiff");
                    PRNR("%.3f", altCalc().sqdiff());
                    break;
                
                case 13:
                    PRNL("Alt dir");
                    switch (altCalc().direct()) {
                        case ACDIR_INIT:    PRNR("init / %d s", altCalc().dirtm()/1000);    break;
                        case ACDIR_ERR:     PRNR("err / %d s",  altCalc().dirtm()/1000);    break;
                        case ACDIR_UP:      PRNR("up / %d s",   altCalc().dirtm()/1000);    break;
                        case ACDIR_FLAT:    PRNR("flat / %d s", altCalc().dirtm()/1000);    break;
                        case ACDIR_DOWN:    PRNR("down / %d s", altCalc().dirtm()/1000);    break;
                    }
                    break;
                
                case 14:
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
                
                case 15:
                    PRNL("Jmp mode");
                    PRNR("%s / %d s",
                        altCalc().jmpmode() == ACJMP_NONE       ? "-" :
                        altCalc().jmpmode() == ACJMP_TAKEOFF    ? "TOFF" :
                        altCalc().jmpmode() == ACJMP_FREEFALL   ? "FF" :
                        altCalc().jmpmode() == ACJMP_CANOPY     ? "CNP" : "-",
                        altCalc().jmptm()/1000
                    );
                    break;
                
                case 16:
                    PRNL("GPS lon");
                    PRNR("%f", NAV_LATLON(gpsInf().lon));
                    break;
                
                case 17:
                    PRNL("GPS lat");
                    PRNR("%f", NAV_LATLON(gpsInf().lat));
                    break;
                
                case 18:
                    PRNL("GPS numSV");
                    PRNR("%u", gpsInf().numSV);
                    break;
                
                case 19:
                    PRNL("GPS gpsFix");
                    PRNR("%u", gpsInf().gpsFix);
                    break;
                
                case 20:
                    PRNL("GPS recv bytes");
                    if (gpsRecv() > 1024*1024)
                        PRNR("%lu M", gpsRecv()/1024/1024);
                    else
                    if (gpsRecv() > 1024)
                        PRNR("%lu k", gpsRecv()/1024);
                    else
                        PRNR("%lu b", gpsRecv());
                    break;
                
                case 21:
                    PRNL("GPS recv error");
                    PRNR("%lu", gpsRecvError());
                    break;
                
                case 22:
                    PRNL("GPS cmd unknown");
                    PRNR("%lu", gpsCmdUnknown());
                    break;
                
                case 23:
                    PRNL("GPS hMSL");
                    PRNR("%.2f m", NAV_MM(gpsInf().hMSL));
                    break;
                
                case 24:
                    PRNL("GPS hAcc");
                    PRNR("%.2f m", NAV_MM(gpsInf().hAcc));
                    break;
                
                case 25:
                    PRNL("GPS vAcc");
                    PRNR("%.2f m", NAV_MM(gpsInf().vAcc));
                    break;
                
                case 26:
                    PRNL("GPS velN");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().velN));
                    break;
                
                case 27:
                    PRNL("GPS velE");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().velE));
                    break;
                
                case 28:
                    PRNL("GPS velD");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().velD));
                    break;
                
                case 29:
                    PRNL("GPS speed");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().speed));
                    break;
                
                case 30:
                    PRNL("GPS gSpeed");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().gSpeed));
                    break;
                
                case 31:
                    PRNL("GPS heading");
                    PRNR("%.1f deg", NAV_DEG(gpsInf().heading));
                    break;
                
                case 32:
                    PRNL("GPS sAcc");
                    PRNR("%.2f m/s", NAV_CM(gpsInf().sAcc));
                    break;
                
                case 33:
                    PRNL("GPS cAcc");
                    PRNR("%.1f deg", NAV_DEG(gpsInf().cAcc));
                    break;
                
                case 34:
                    PRNL("GPS date");
                    PRNR("%d.%02d.%02d", gpsInf().tm.day, gpsInf().tm.mon, gpsInf().tm.year);
                    break;
                
                case 35:
                    PRNL("GPS time");
                    PRNR("%d:%02d:%02d.%d", gpsInf().tm.h, gpsInf().tm.m, gpsInf().tm.s, gpsInf().tm.tick);
                    break;
                
                case 36:
                    PRNL("Sys date");
                    PRNR("%d.%02d.%02d", tmNow().day, tmNow().mon, tmNow().year);
                    break;
                
                case 37:
                    PRNL("Sys time");
                    PRNR("%d:%02d:%02d.%d", tmNow().h, tmNow().m, tmNow().s, tmNow().tick);
                    break;
                
                case 38:
                    PRNL("Sys tm valid");
                    PRNR("%d", tmValid() ? 1 : 0);
                    break;
                
                case 39: {
                        uint32_t tm = tmcntInterval(TMCNT_UPTIME);
                        uint32_t m = tm / 60;
                        uint32_t h = m / 60;
                        PRNL("Uptime");
                        PRNR("%d:%02d:%02d", h, m % 60, tm % 60);
                    }
                    break;
                
                case 40: {
                        uint32_t tm = tmcntInterval(TMCNT_NOFLY);
                        uint32_t m = tm / 60;
                        uint32_t h = m / 60;
                        PRNL("No-Fly time");
                        PRNR("%d:%02d:%02d", h, m % 60, tm % 60);
                    }
                    break;
                
                case 41:
                    PRNL("Compas Head");
                    PRNR("%0.1f", compass().head);
                    break;
                
                case 42:
                    PRNL("Compas H-Speed");
                    PRNR("%0.3f", compass().speed * 1000);
                    break;
                
                case 43:
                    PRNL("Magnetic X");
                    PRNR("%d", compass().mag.x);
                    break;
                
                case 44:
                    PRNL("Magnetic Y");
                    PRNR("%d", compass().mag.y);
                    break;
                
                case 45:
                    PRNL("Magnetic Z");
                    PRNR("%d", compass().mag.z);
                    break;
                
                case 46:
                    PRNL("Mag err X");
                    PRNR("%d", compass().merr.x);
                    break;
                
                case 47:
                    PRNL("Mag err Y");
                    PRNR("%d", compass().merr.y);
                    break;
                
                case 48:
                    PRNL("Mag err Z");
                    PRNR("%d", compass().merr.z);
                    break;
                
                case 49:
                    PRNL("Accel X");
                    PRNR("%d", compass().acc.x);
                    break;
                
                case 50:
                    PRNL("Accel Y");
                    PRNR("%d", compass().acc.y);
                    break;
                
                case 51:
                    PRNL("Accel Z");
                    PRNR("%d", compass().acc.z);
                    break;
                
                case 52:
                    PRNL("E X");
                    PRNR("%ld", compass().e.x);
                    break;
                
                case 53:
                    PRNL("E Y");
                    PRNR("%ld", compass().e.y);
                    break;
                
                case 54:
                    PRNL("E Z");
                    PRNR("%ld", compass().e.z);
                    break;
            }
        }
};
static ViewInfoDebug vInfDebug;
void setViewInfoDebug() { viewSet(vInfDebug); }

#endif // FWVER_DEBUG
