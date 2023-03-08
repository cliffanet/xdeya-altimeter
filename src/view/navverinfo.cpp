
#include <vector>

#include "info.h"
#include "main.h"

#include "../navi/proc.h"
#include "../navi/ubloxproto.h"

#include "../log.h"

static void gpsRecvVer(UbloxGpsProto &gps);

class ViewNavVerInfo : public ViewInfo {
    std::vector<char *> strall;

    public:
        ViewNavVerInfo() : ViewInfo(0) {}

        void addstr(const char *str) {
            char *s = strdup(str);
            if (s != NULL)
                strall.push_back(s);
            setSize(strall.size());
        }
        
        void start() {
            // Почему-то у этой команды в ответе проблемы с ck-sum,
            // поэтому отдельно задаём флаг "игнорировать cksum"
            bool ok = gpsProto()->get(UBX_MON, UBX_MON_VER, gpsRecvVer, true);
            CONSOLE("get: %d", ok);
        }

        void stop() {
            for (auto *s: strall)
                free(s);
            strall.clear();
        }
        
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL) {
                ViewInfo::btnSmpl(btn);
                return;
            }
            
            stop();
            setViewMain();
        }
        
        void updStr(uint16_t i) {
            if (i > strall.size())
                return;
            prnstr(strall[i]);
        }
};
static ViewNavVerInfo vNavVerInfo;
void setViewNavVerInfo() {
    viewSet(vNavVerInfo);
    vNavVerInfo.start();
}

static bool gpsStr(UbloxGpsProto &gps, uint16_t &i, uint8_t len) {
    char s[len+1];
    //CONSOLE("plen: %d, i: %d, len: %d", gps.plen(), i, len);

    if (i >= gps.plen())
        return false;

    gps.bufcopy(reinterpret_cast<uint8_t *>(s), len, i);
    s[len] = '\0';
    CONSOLE("str: %s", s);
    i += len;

    vNavVerInfo.addstr(s);
    return true;
}

static void gpsRecvVer(UbloxGpsProto &gps) {
    uint16_t i = 0;
    if (!gpsStr(gps, i, 30))
        return;
    if (!gpsStr(gps, i, 10))
        return;
    while (gpsStr(gps, i, 30)) {}
    
}
