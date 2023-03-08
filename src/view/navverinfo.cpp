
#include <vector>

#include "info.h"
#include "menu.h"

#include "../navi/proc.h"
#include "../navi/ubloxproto.h"

#include "../log.h"

static void gpsRecvVer(UbloxGpsProto &gps);
class ViewNavVerInfo;
static ViewNavVerInfo *v = NULL;

class ViewNavVerInfo : public ViewInfo {
    std::vector<char *> strall;

    public:
        ViewNavVerInfo() : ViewInfo(0) {
            if (v == NULL)
                v = this;
            // Почему-то у этой команды в ответе проблемы с ck-sum,
            // поэтому отдельно задаём флаг "игнорировать cksum"
            bool ok = gpsProto()->get(UBX_MON, UBX_MON_VER, gpsRecvVer, true);
            CONSOLE("get: %d", ok);
        }
        ~ViewNavVerInfo() {
            for (auto *s: strall)
                free(s);
            strall.clear();
            if (v == this)
                v = NULL;
            CONSOLE("end");
        }

        static void addstr(const char *str) {
            if (v == NULL)
                return;
            
            char *s = strdup(str);
            if (s != NULL)
                v->strall.push_back(s);
            v->setSize(v->strall.size());
        }
        
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL) {
                ViewInfo::btnSmpl(btn);
                return;
            }
            
            menuRestore();
        }
        
        void updStr(uint16_t i) {
            if (i > strall.size())
                return;
            prnstr(strall[i]);
        }
};

void setViewNavVerInfo() { viewOpen<ViewNavVerInfo>(); }

static bool gpsStr(UbloxGpsProto &gps, uint16_t &i, uint8_t len) {
    char s[len+1];
    //CONSOLE("plen: %d, i: %d, len: %d", gps.plen(), i, len);

    if (i >= gps.plen())
        return false;

    gps.bufcopy(reinterpret_cast<uint8_t *>(s), len, i);
    s[len] = '\0';
    CONSOLE("str: %s", s);
    i += len;

    ViewNavVerInfo::addstr(s);
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
