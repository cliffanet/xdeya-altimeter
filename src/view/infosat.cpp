
#include <vector>

#include "info.h"
#include "main.h"

#include "../navi/proc.h"
#include "../navi/ubloxproto.h"

#include "../log.h"

typedef struct {
        uint8_t gnssId;     // GNSS identifier
        uint8_t svId;       // Satellite identifier
        uint8_t cno;        // [dBHz] Carrier to noise ratio (signal strength)
        int8_t  elev;       // [deg] Elevation (range: +/-90), unknown if out of range
        int16_t azim;       // [deg] Azimuth (range 0-360), unknown if elevation is out of range
        int16_t prRes;      // [m] Pseudorange residual
        uint32_t flags;     // Bitmask
    } sat_item_t;

/* Bitfield flags

qualityInd Signal quality indicator:
0: no signal
1: searching signal
2: signal acquired
3: signal detected but unusable
4: code locked and time synchronized
5, 6, 7: code and carrier locked and time synchronized
Note: Since IMES signals are not time synchronized, a channel tracking an IMES signal can never
reach a quality indicator value of higher than 3.

svUsed 1 = Signal in the subset specified in Signal Identifiers is currently being used for navigation

health Signal health flag:
0: unknown
1: healthy
2: unhealthy

diffCorr 1 = differential correction data is available for this SV

smoothed 1 = carrier smoothed pseudorange used

orbitSource Orbit source:
0: no orbit information is available for this SV
1: ephemeris is used
2: almanac is used
3: AssistNow Offline orbit is used
4: AssistNow Autonomous orbit is used
5, 6, 7: other orbit information is used

ephAvail 1 = ephemeris is available for this SV

almAvail 1 = almanac is available for this SV

anoAvail 1 = AssistNow Offline data is available for this SV

aopAvail 1 = AssistNow Autonomous data is available for this SV

sbasCorrUsed 1 = SBAS corrections have been used for a signal in the subset specified in Signal Identifiers

rtcmCorrUsed 1 = RTCM corrections have been used for a signal in the subset specified in Signal Identifiers

slasCorrUsed 1 = QZSS SLAS corrections have been used for a signal in the subset specified in Signal Identifiers

spartnCorrUsed
1 = SPARTN corrections have been used for a signal in the subset specified in Signal Identifiers

prCorrUsed 1 = Pseudorange corrections have been used for a signal in the subset specified in Signal Identifiers

crCorrUsed 1 = Carrier range corrections have been used for a signal in the subset specified in Signal Identifiers

doCorrUsed 1 = Range rate (Doppler) corrections have been used for a signal in the subset specified in Signal Identifiers

clasCorrUsed 1 = CLAS corrections have been used for a signal in the subset specified in Signal Identifiers

*/

static std::vector<sat_item_t> satall;
static size_t recv_plen = 0, recv_cnt = 0;

static void gpsRecvSat(UbloxGpsProto &gps);

class ViewInfoSat : public ViewInfo {
    public:
        ViewInfoSat() : ViewInfo(1) {}
        
        void start() {
            const auto &gps = gpsProto();
            gps->hndadd(UBX_NAV, UBX_NAV_SAT, gpsRecvSat);
            ubx_cfg_rate_t r = { UBX_NAV, UBX_NAV_SAT, 5 };
            gps->send(UBX_CFG, UBX_CFG_MSG, r);
        }
        
        void stop() {
            const auto &gps = gpsProto();
            gps->hnddel(UBX_NAV, UBX_NAV_SAT, gpsRecvSat);
            ubx_cfg_rate_t r = { UBX_NAV, UBX_NAV_SAT, 0 };
            gps->send(UBX_CFG, UBX_CFG_MSG, r);
            satall.clear();
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
            if (i == 0) {
                PRNL("plen: %d; count: %d; upd: %d", recv_plen, satall.size(), recv_cnt);
                return;
            }
            
            if (i > satall.size())
                return;
            
            const auto &s = satall[i-1];
            const char *id =
                (s.svId >= 1) && (s.svId <= 32) ?
                    PSTR("GPS ") :
                (s.svId >= 120) && (s.svId <= 158) ?
                    PSTR("SBAS") :
                (s.svId >= 211) && (s.svId <= 246) ?
                    PSTR("Gali") :
                ((s.svId >= 33) && (s.svId <= 64)) || ((s.svId >= 159) && (s.svId <= 163)) ?
                    PSTR("BeiD") :
                (s.svId >= 173) && (s.svId <= 182) ?
                    PSTR("IMES") :
                (s.svId >= 193) && (s.svId <= 202) ?
                    PSTR("QZSS") :
                ((s.svId >= 65) && (s.svId <= 96)) || (s.svId == 255)?
                    PSTR("GLON") :
                    PSTR("    ");
            char ids[10];
            strncpy_P(ids, id, sizeof(ids)-1);
            
#if HWVER >= 4
            PRNL("[%02x %s] q-%d %c (%d/%d)", s.svId, ids, s.flags&0x7, s.flags&0x8 ? '*' : ' ',
                        s.elev, s.azim);
#else
            PRNL("%02x %s q-%d %c", s.svId, ids, s.flags&0x7, s.flags&0x8 ? '*' : ' ');
#endif
            if (s.cno)
                PRNR("%u dB", s.cno);
        }
};
static ViewInfoSat vInfSat;
void setViewInfoSat() {
    viewSet(vInfSat);
    vInfSat.start();
}



static void gpsRecvSat(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;      // GPS time of week             (ms)
    	uint8_t  version;   // Message version (0x01 for this version)
    	uint8_t  numSvs;    // Number of satellites
    	uint8_t  _[2];      // Reserved
    } hdr;
    
    recv_cnt++;
    recv_plen = gps.plen();
    
    if (!gps.bufcopy(hdr))
        return;
    
    satall.resize(hdr.numSvs);
    
    vInfSat.setSize(hdr.numSvs + 1);
    
    uint8_t n = 0;
    for (auto &s: satall) {
        gps.bufcopy(reinterpret_cast<uint8_t *>(&s), sizeof(s), sizeof(hdr)+(n*sizeof(s)));
        n++;
    }
}
