
#include "proc.h"
#include "../log.h"
#include "../core/worker.h"
#include "RTClib.h" // DateTime class
#include "../cfg/main.h" // timezone

// будем использовать стандартный экземпляр класса HardwareSerial, 
// т.к. он и так в системе уже есть и память под него выделена
// Стандартные пины для свободного аппаратного Serial2: 16(rx), 17(tx)
#include <Arduino.h> // Serial, delay
#define ss Serial2

#include "ubloxproto.h"
static UbloxGpsProto gps(ss, 1024);

static gps_data_t data = { 0 };

static bool direct = false;
static gps_state_t state = NAV_STATE_OFF;

static struct {
    uint8_t posllh  : 4;
    uint8_t velned  : 4;
    uint8_t timeutc : 4;
    uint8_t sol     : 4;
    uint8_t pvt     : 4;
} ageRecv = {
    posllh  : 15,
    velned  : 15,
    timeutc : 15,
    sol     : 15,
    pvt     : 15,
};

const bool gps_data_s::disAcc() const {
    return cfg.d().navnoacc;
}


/* ------------------------------------------------------------------------------------------- *
 *  NAV-инициализация
 *  т.к. инициализация довольно долгая (около 500мс), то делаем её через Worker
 * ------------------------------------------------------------------------------------------- */
static void gpsRecvPosllh   (UbloxGpsProto &gps);
static void gpsRecvVelned   (UbloxGpsProto &gps);
static void gpsRecvTimeUtc  (UbloxGpsProto &gps);
static void gpsRecvSol      (UbloxGpsProto &gps);
static void gpsRecvPvt      (UbloxGpsProto &gps);

#define gpsnd(cl, cmd, ...) do { \
        if (!gps.send(cl, cmd, ##__VA_ARGS__)) \
                return errsnd(); \
    } while (0)

class _naviInit : public Wrk {
    state_t errsnd() {
        CONSOLE("NAV config-send (line: %d) fail", __line);
        state = NAV_STATE_FAIL;
        return END;
    }

    uint8_t m_cnfcnt = 0;
    state_t cnfwait() {
        // всё ещё ждём
        m_cnfcnt ++;
        if (m_cnfcnt > 50) {
            CONSOLE("Wait cmd-confirm (line: %d, cnfneed: %d) timeout", __line, gps.cnfneed());
            state = NAV_STATE_FAIL;
            return END;
        }
        //CONSOLE("Wait cmd-confirm (op: %d, cnfneed: %d, cnfcnt: %d)", m_op, gps.cnfneed(), m_cnfcnt);
        return DLY;
    }

public:
#ifdef FWVER_DEBUG
    ~_naviInit() {
        CONSOLE("_naviInit(0x%08x) destroy", this);
    }
#endif

    state_t run() {
        if (!gps.tick()) {
            CONSOLE("Wait cmd-confirm fail");
            state = NAV_STATE_FAIL;
            return END;
        }

    WPROC
        CONSOLE("Navi init begin");
        state = NAV_STATE_INIT;
    
        // инициируем uart-порт NAV-приёмника
        ss.begin(9600);
        ss.setRxBufferSize(512); // По умолчанию = 256 и этого не хватает, чтобы принять сразу все присылаемые от Navi данные за один цикл
        
        gps.hndclear();
        gps.hndadd(UBX_NAV,  UBX_NAV_POSLLH,     gpsRecvPosllh);
        gps.hndadd(UBX_NAV,  UBX_NAV_VELNED,     gpsRecvVelned);
        gps.hndadd(UBX_NAV,  UBX_NAV_TIMEUTC,    gpsRecvTimeUtc);
        gps.hndadd(UBX_NAV,  UBX_NAV_SOL,        gpsRecvSol);
        gps.hndadd(UBX_NAV,  UBX_NAV_PVT,        gpsRecvPvt);
        // Пустое ожидание сразу после запуска процесса инициализации.
        // Необходимо, чтобы дождаться инициализации чипа навигации сразу после подачи питания
    WPRC_DLY
        
        CONSOLE("Set UART(gps) default speed 9600");
        ss.updateBaudRate(9600);
    WPRC_DLY
        
        const struct {
        	uint8_t  portID;       // Port identifier number
        	uint8_t  reserved0;    // Reserved
        	uint16_t txReady;      // TX ready pin configuration
        	uint32_t mode;         // UART mode
        	uint32_t baudRate;     // Baud rate (bits/sec)
        	uint16_t inProtoMask;  // Input protocols
        	uint16_t outProtoMask; // Output protocols
        	uint16_t flags;        // Flags
        	uint16_t reserved5;    // Always set to zero
        } cfg_prt =
    	{
    		.portID       = 1,      // UART 1
    		.reserved0    = 0,      // Reserved
    		.txReady      = 0,      // no TX ready
    		.mode         = 0x08d0, // 8N1
    		.baudRate     = 57600,  // Baudrate in bits/second
    		.inProtoMask  = 0x0001, // UBX protocol
    		.outProtoMask = 0x0001, // UBX protocol
    		.flags        = 0,      // Flags bit mask
    		.reserved5    = 0       // Reserved, set to 0
    	};
        gpsnd(UBX_CFG, UBX_CFG_PRT, cfg_prt);
    WPRC_DLY
        
        gps.cnfclear();
        gps.rcvclear();
        ss.flush();
        while (ss.available()) ss.read();

        CONSOLE("Set UART(gps) speed 57600");
        ss.updateBaudRate(57600);
    WPRC_DLY
        
        const struct {
        	uint8_t  portID;       // Port identifier number
        	uint8_t  reserved0;    // Reserved
        	uint16_t txReady;      // TX ready pin configuration
        	uint32_t mode;         // UART mode
        	uint32_t baudRate;     // Baud rate (bits/sec)
        	uint16_t inProtoMask;  // Input protocols
        	uint16_t outProtoMask; // Output protocols
        	uint16_t flags;        // Flags
        	uint16_t reserved5;    // Always set to zero
        } cfg_prt =
    	{
    		.portID       = 1,      // UART 1
    		.reserved0    = 0,      // Reserved
    		.txReady      = 0,      // no TX ready
    		.mode         = 0x08d0, // 8N1
    		.baudRate     = 57600,  // Baudrate in bits/second
    		.inProtoMask  = 0x0001, // UBX protocol
    		.outProtoMask = 0x0001, // UBX protocol
    		.flags        = 0,      // Flags bit mask
    		.reserved5    = 0       // Reserved, set to 0
    	};
        gpsnd(UBX_CFG, UBX_CFG_PRT, cfg_prt);
    WPRC_RUN
        
        if (gps.cnfneed() > 0)
            return cnfwait();
        m_cnfcnt = 0;
        CONSOLE("Confirmed speed");
    WPRC_RUN
        
        const ubx_cfg_rate_t cfg_rate[] = {
    		{ UBX_NMEA, UBX_NMEA_GPGGA,     0 },
    		{ UBX_NMEA, UBX_NMEA_GPGLL,     0 },
    		{ UBX_NMEA, UBX_NMEA_GPGSA,     0 },
    		{ UBX_NMEA, UBX_NMEA_GPGSV,     0 },
    		{ UBX_NMEA, UBX_NMEA_GPRMC,     0 },
    		{ UBX_NMEA, UBX_NMEA_GPVTG,     0 },
        
    		{ UBX_NAV,  UBX_NAV_POSLLH,     1 },
    		{ UBX_NAV,  UBX_NAV_VELNED,     1 },
    		{ UBX_NAV,  UBX_NAV_TIMEUTC,    1 },
        
            { UBX_NAV,  UBX_NAV_SOL,        1 },
            { UBX_NAV,  UBX_NAV_PVT,        1 }
        };
        
        for (auto &rate: cfg_rate)
            gpsnd(UBX_CFG, UBX_CFG_MSG, rate);
    WPRC_RUN
        
        const struct {
        	uint16_t measRate;  // Measurement rate             (ms)
        	uint16_t navRate;   // Nagivation rate, in number 
        	                       //   of measurement cycles
        	uint16_t timeRef;   // Alignment to reference time:
        	                       //   0 = UTC time; 1 = NAV time
        } cfg_mrate = {
    		.measRate   = 200,      // Measurement rate (ms)
    		.navRate    = 1,        // Navigation rate (cycles)
    		.timeRef    = 0         // UTC time
    	};
        gpsnd(UBX_CFG, UBX_CFG_RATE, cfg_mrate);
    WPRC_RUN

        if (!gpsUpdateCfgModel())
            return errsnd();
    WPRC_RUN
        
        if (!gpsUpdateCfgGNSS())
            return errsnd();
    WPRC_RUN
        
        if (gps.cnfneed() > 0)
            return cnfwait();
        
        const struct {
        	uint16_t navBbrMask; // BBR sections to clear
        	uint8_t  resetMode;  // Reset type
        	uint8_t  res;        // Reserved
        } cfg_rst =	{
    		.navBbrMask = 0x0000,   // Hot start
    		.resetMode  = 0x09      // Controlled NAV start
    	};
        // В документации пишут, что на старых версиях прошивки подтверждение этой
        // команды приходит нестабильно, а в новых - специально убрали подтверждение
        // этой команды, т.к. всё равно, до перезагрузки устройства это подтверждение
        // не успевает отправиться
        gpsnd(UBX_CFG, UBX_CFG_RST, cfg_rst);
        gps.cnfclear();
        
        state = NAV_STATE_OK;
        CONSOLE("NAV-UART config ok");

    WPRC(END)
    }
};

#undef gpsnd

static WrkProc<_naviInit> _init;
void naviInit() {
    if (!_init.isrun())
        _init = wrkRun<_naviInit>();
}

/* ------------------------------------------------------------------------------------------- *
 *  NAV-получение данных
 * ------------------------------------------------------------------------------------------- */

static void gpsRecvPosllh(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;     // NAV time of week             (ms)
    	int32_t  lon;      // Longitude                    (deg)
    	int32_t  lat;      // Latitude                     (deg)
    	int32_t  height;   // Height above ellipsoid       (mm)
    	int32_t  hMSL;     // Height above mean sea level  (mm)
    	uint32_t hAcc;     // Horizontal accuracy estimate (mm)
    	uint32_t vAcc;     // Vertical accuracy estimate   (mm)
    } nav;
    
    if (!gps.bufcopy(nav))
        return;
    
    data.lon    = nav.lon;
	data.lat    = nav.lat;
	data.hMSL   = nav.hMSL;
	data.hAcc   = nav.hAcc;
	data.vAcc   = nav.vAcc;
    ageRecv.posllh = 0;
}
static void gpsRecvVelned(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;     // NAV time of week             (ms)
    	int32_t  velN;     // North velocity               (cm/s)
    	int32_t  velE;     // East velocity                (cm/s)
    	int32_t  velD;     // Down velocity                (cm/s)
    	uint32_t speed;    // 3D speed                     (cm/s)
    	uint32_t gSpeed;   // Ground speed                 (cm/s)
    	int32_t  heading;  // 2D heading                   (deg)
    	uint32_t sAcc;     // Speed accuracy estimate      (cm/s)
    	uint32_t cAcc;     // Heading accuracy estimate    (deg)
    } nav;
    
    if (!gps.bufcopy(nav))
        return;
    
    data.velN   = nav.velN;
    data.velE   = nav.velE;
    data.velD   = nav.velD;
    data.speed  = nav.speed;
    data.gSpeed = nav.gSpeed;
    data.heading= nav.heading;
    data.sAcc   = nav.sAcc;
    data.cAcc   = nav.cAcc;
    ageRecv.velned = 0;
}
static void gpsRecvTimeUtc(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;     // NAV time of week             (ms)
    	uint32_t tAcc;     // Time accuracy estimate       (ns)
    	int32_t  nano;     // Nanoseconds of second        (ns)
    	uint16_t year;     // Year                         (1999..2099)
    	uint8_t  month;    // Month                        (1..12)
    	uint8_t  day;      // Day of month                 (1..31)
    	uint8_t  hour;     // Hour of day                  (0..23)
    	uint8_t  min;      // Minute of hour               (0..59)
    	uint8_t  sec;      // Second of minute             (0..59)
    	uint8_t  valid;    // Validity flags
    } nav;
    
    if (!gps.bufcopy(nav))
        return;
    
    data.tm.year= nav.year;
    data.tm.mon = nav.month;
    data.tm.day = nav.day;
    data.tm.h   = nav.hour;
    data.tm.m   = nav.min;
    data.tm.s   = nav.sec;
    data.tm.tick= nav.nano / 1000000 / TIME_TICK_INTERVAL;
    ageRecv.timeutc = 0;
}
static void gpsRecvSol(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;     // NAV time of week             (ms)
    	int32_t  fTOW;     // Fractional nanoseconds       (ns)
    	int16_t  week;     // NAV week
    	uint8_t  gpsFix;   // NAV fix type
    	uint8_t  flags;    // Fix status flags
    	int32_t  ecefX;    // ECEF X coordinate            (cm)
    	int32_t  ecefY;    // ECEF Y coordinate            (cm)
    	int32_t  ecefZ;    // ECEF Z coordinate            (cm)
    	uint32_t pAcc;     // 3D position accuracy         (cm)
    	int32_t  ecefVX;   // ECEF X velocity              (cm/s)
    	int32_t  ecefVY;   // ECEF Y velocity              (cm/s)
    	int32_t  ecefVZ;   // ECEF Z velocity              (cm/s)
    	uint32_t sAcc;     // Speed accuracy               (cm/s)
    	uint16_t pDOP;     // Position DOP
    	uint8_t  res1;     // Reserved
    	uint8_t  numSV;    // Number of SVs in solution
    	uint32_t res2;     // Reserved
    } nav;
    
    if (!gps.bufcopy(nav))
        return;
    
    data.gpsFix = nav.gpsFix;
    data.numSV  = nav.numSV;
    ageRecv.sol = 0;
}
static void gpsRecvPvt(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;         // NAV time of week              (ms)
    	uint16_t year;         // Year                          (1999..2099)
    	uint8_t  month;        // Month                         (1..12)
    	uint8_t  day;          // Day of month                  (1..31)
    	uint8_t  hour;         // Hour of day                   (0..23)
    	uint8_t  min;          // Minute of hour                (0..59)
    	uint8_t  sec;          // Second of minute              (0..59)
    	uint8_t  valid;        // Validity flags
    	uint32_t tAcc;         // Time accuracy estimate        (ns)
    	int32_t  nano;         // Nanoseconds of second         (ns)
    	uint8_t  gpsFix;       // NAV fix type
    	uint8_t  flags;        // Fix status flags
    	uint8_t  flags2;       // Additional flags
    	uint8_t  numSV;        // Number of SVs in solution
    	int32_t  lon;          // Longitude                     (deg)
    	int32_t  lat;          // Latitude                      (deg)
    	int32_t  height;       // Height above ellipsoid        (mm)
    	int32_t  hMSL;         // Height above mean sea level   (mm)
    	uint32_t hAcc;         // Horizontal accuracy estimate  (mm)
    	uint32_t vAcc;         // Vertical accuracy estimate    (mm)
    	int32_t  velN;         // North velocity                (mm/s)
    	int32_t  velE;         // East velocity                 (mm/s)
    	int32_t  velD;         // Down velocity                 (mm/s)
    	int32_t  gSpeed;       // Ground speed                  (mm/s)
    	int32_t  headMot;      // 2D heading of motion          (deg)
    	uint32_t sAcc;         // Speed accuracy estimate       (mm/s)
    	uint32_t headAcc;      // Heading accuracy estimate     (deg)
    	uint16_t pDOP;         // Position DOP
    	uint8_t  flags3;       // Additional flags
    	uint8_t  reserved0[5]; // Reserved
    	int32_t  headVeh;      // 2D heading of vehicle         (deg)
    	int16_t  magDec;       // Magnetic declination          (deg)
    	uint16_t magAcc;       // Magnetic declination accuracy (deg)
    } nav;
    
    if (!gps.bufcopy(nav))
        return;
    
	data.gpsFix = nav.gpsFix;
	data.numSV  = nav.numSV;
    ageRecv.pvt = 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  Данные о NAV для внешнего использования
 * ------------------------------------------------------------------------------------------- */

UbloxGpsProto * gpsProto() { return &gps; }
const gps_data_t &gpsInf() { return data; };

uint32_t gpsRecv() { return gps.cntRecv(); }
uint32_t gpsRecvError() { return gps.cntRecvErr(); }
uint32_t gpsCmdUnknown() { return gps.cntCmdUnknown(); }
uint8_t gpsDataAge() {
    uint32_t frst = ageRecv.posllh;
    if (frst < ageRecv.velned)
        frst = ageRecv.velned;
    if (frst < ageRecv.timeutc)
        frst = ageRecv.timeutc;
    if (frst < ageRecv.sol)
        frst = ageRecv.sol;
    if (frst < ageRecv.pvt)
        frst = ageRecv.pvt;
    return frst;
}
gps_state_t gpsState() { return state; }


/* ------------------------------------------------------------------------------------------- *
 *  NAV-инициализация
 * ------------------------------------------------------------------------------------------- */
static void gpsFree() {
    gps.uart(NULL);
}

static bool gpsCmdConfirm() {
    uint16_t cnt = 0;
    while (1) {
        cnt ++;
        if (!gps.tick()) {
            CONSOLE("Wait cmd-confirm fail");
            return false;
        }
        if (gps.cnfneed() == 0)
            return true;
        if (cnt > 20) {
            CONSOLE("Wait cmd-confirm timeout");
            return false;
        }
        delay(50);
    }
    
    return false;
}

#ifdef FWVER_DEBUG
static void gpsRecvGnss(UbloxGpsProto &gps) {
    struct {
    	uint8_t  msgVer;        // Message version (0x00 for this version)
    	uint8_t  numTrkChHw;    // Number of tracking channels available in hardware (read only)
    	uint8_t  numTrkChUse;   // (Read only in protocol versions greater
                                // than 23) Number of tracking channels to
                                // use. Must be > 0, <= numTrkChHw. If
                                // 0xFF, then number of tracking channels to
                                // use will be set to numTrkChHw.
    	uint8_t  numConfigBlocks; // Number of configuration blocks following
    } hdr;
    
    if (!gps.bufcopy(hdr))
        return;
    
    struct {
    	uint8_t  gnssId;        // System identifier
    	uint8_t  resTrkCh;      // (Read only in protocol versions greater
                                // than 23) Number of reserved (minimum)
                                // tracking channels for this system.
    	uint8_t  maxTrkCh;      // (Read only in protocol versions greater
                                // than 23) Maximum number of tracking
                                // channels used for this system. Must be >
                                // 0, >= resTrkChn, <= numTrkChUse and <=
                                // maximum number of tracking channels
                                // supported for this system.
    	uint8_t  _;             // Reserved
        uint32_t flags;         // Bitfield of flags. At least one signal must
                                // be configured in every enabled system.
    } b;
    
    uint8_t n = 0;
    CONSOLE("GNSS config: %u items; numTrkChHw=%u, numTrkChUse=%u",
        hdr.numConfigBlocks, hdr.numTrkChHw, hdr.numTrkChUse);
    while (hdr.numConfigBlocks > 0) {
        gps.bufitem(hdr, b, n);
        CONSOLE("[0x%02x %s] resTrkCh=%u, maxTrkCh=%u, en=%d, sigCfgMask=0x%02x",
                b.gnssId,
                b.gnssId == 0 ? "GPS" :
                b.gnssId == 1 ? "SBAS" :
                b.gnssId == 2 ? "Galileo" :
                b.gnssId == 3 ? "BeiDou" :
                b.gnssId == 4 ? "IMES" :
                b.gnssId == 5 ? "QZSS" :
                b.gnssId == 6 ? "GLONASS" : "-",
                b.resTrkCh, b.maxTrkCh,
                b.flags&0x01, (b.flags>>16)&0xff);
        
        hdr.numConfigBlocks --;
        n++;
    }
}

static void gpsRecvModel(UbloxGpsProto &gps) {
    struct {
        uint16_t mask;              // Only masked parameters will be applied
        uint8_t  dynModel;          // Dynamic platform model
        uint8_t  fixMode;           // Position fixing mode
        int32_t  fixedAlt;          // Fixed altitude (MSL) for 2D mode       (m)
        uint32_t fixedAltVar;       // Fixed altitude variance for 2D mode    (m^2)
        int8_t   minElev;           // Minimum elevation for satellite        (deg)
        uint8_t  drLimit;           // Maximum time to perform dead reckoning (s)
        uint16_t pDop;              // Position DOP mask
        uint16_t tDop;              // Time DOP mask
        uint16_t pAcc;              // Position accuracy mask                 (m)
        uint16_t tAcc;              // Time accuracy mask                     (m)
        uint8_t  staticHoldThresh;  // Static hold threshold                  (cm/s)
        uint8_t  res1;              // Reserved, set to 0
        uint32_t res2;              // Reserved, set to 0
        uint32_t res3;              // Reserved, set to 0
        uint32_t res4;              // Reserved, set to 0
    } m;

    if (!gps.bufcopy(m))
        return;
    
    CONSOLE("Model config: dynModel=%u, fixMode=%u",
        m.dynModel, m.fixMode);
}
#endif // FWVER_DEBUG

static void gpsDirectToSerial(uint8_t c) {
    Serial.write( c );
}

void gpsProcess() {
    if (ageRecv.posllh  < 15)
        ageRecv.posllh  ++;
    if (ageRecv.velned  < 15)
        ageRecv.velned  ++;
    if (ageRecv.timeutc < 15)
        ageRecv.timeutc ++;
    if (ageRecv.sol     < 15)
        ageRecv.sol     ++;
    if (ageRecv.pvt     < 15)
        ageRecv.pvt     ++;
    
    gps.tick(direct ? &gpsDirectToSerial : NULL);
    
    while (direct && Serial.available())
        ss.write( Serial.read() );

    data.rcvok =
        (ageRecv.posllh     < 7) &&
        (ageRecv.velned     < 7) &&
        (ageRecv.timeutc    < 7) &&
        (ageRecv.sol        < 7) &&
        (ageRecv.pvt        < 7);
    
    if (state >= NAV_STATE_NODATA)
        state = data.rcvok ? NAV_STATE_OK : NAV_STATE_NODATA;
    
    if (_init.valid() && !_init.isrun())
        _init.reset();
}

/* ------------------------------------------------------------------------------------------- *
 *  Жёсткая перезагрузка с очисткой списка спутников
 * ------------------------------------------------------------------------------------------- */
bool gpsColdRestart() {
    if (_init.isrun())
        return false;
    
    const struct {
    	uint16_t navBbrMask; // BBR sections to clear
    	uint8_t  resetMode;  // Reset type
    	uint8_t  res;        // Reserved
    } cfg_rst =	{
		.navBbrMask = 0xFFFF,   // Cold start
		.resetMode  = 0x00      // Hardware reset (watchdog) immediately
	};
    
    if (!gps.send(UBX_CFG, UBX_CFG_RST, cfg_rst))
        return false;
    gps.cnfclear();
    
    delay(1000);
    
    naviInit();
    
    return true; 
}

/* ------------------------------------------------------------------------------------------- *
 *  Обновление режима GPS/GLONASS
 * ------------------------------------------------------------------------------------------- */
bool gpsUpdateCfgGNSS() {
    CONSOLE("update gnss");

    typedef struct __attribute__((__packed__)) {
    	uint8_t  msgVer;        // Message version (0x00 for this version)
    	uint8_t  numTrkChHw;    // Number of tracking channels available in hardware (read only)
    	uint8_t  numTrkChUse;   // (Read only in protocol versions greater
                                // than 23) Number of tracking channels to
                                // use. Must be > 0, <= numTrkChHw. If
                                // 0xFF, then number of tracking channels to
                                // use will be set to numTrkChHw.
    	uint8_t  numConfigBlocks; // Number of configuration blocks following
    } hdr_t;
    
    typedef struct __attribute__((__packed__)) {
    	uint8_t  gnssId;        // System identifier
    	uint8_t  resTrkCh;      // (Read only in protocol versions greater
                                // than 23) Number of reserved (minimum)
                                // tracking channels for this system.
    	uint8_t  maxTrkCh;      // (Read only in protocol versions greater
                                // than 23) Maximum number of tracking
                                // channels used for this system. Must be >
                                // 0, >= resTrkChn, <= numTrkChUse and <=
                                // maximum number of tracking channels
                                // supported for this system.
    	uint8_t  _;             // Reserved
        uint32_t flags;         // Bitfield of flags. At least one signal must
                                // be configured in every enabled system.
    } sys_t;

    const struct {
        bool glo, gps, all;
    } use {
        .glo    = ((cfg.d().navgnss & 0x1) > 0) || ((cfg.d().navgnss & 0x4) > 0),
        .gps    = ((cfg.d().navgnss & 0x2) > 0) || ((cfg.d().navgnss & 0x4) > 0),
        .all    = (cfg.d().navgnss & 0x4) > 0,
    };
    CONSOLE("cfg navgnss: %d; glo: %d, gps: %d, all: %d", cfg.d().navgnss, use.glo, use.gps, use.all);
    
    const struct {
        hdr_t hdr;
        sys_t gps, sbas, galileo, beidoo, imes, qzss, glon;
    } data = {
        .hdr    = {
            .msgVer     = 0x00,
            .numTrkChHw = 32,
            .numTrkChUse= 0xFF,
            .numConfigBlocks=7,
        },
        .gps    = {
            .gnssId     = 0x00,
            .resTrkCh   = 8,
            .maxTrkCh   = 32,//static_cast<uint8_t>(use.glo ? 16 : 32),
            ._          = 0,
            .flags      = (0x01<<16) | (use.gps ? 0x1 : 0x0),
        },
        .sbas    = {
            .gnssId     = 0x01,
            .resTrkCh   = 4,
            .maxTrkCh   = 8,
            ._          = 0,
            .flags      = (0x01<<16) | (use.all ? 0x1 : 0x0),
        },
        .galileo    = {
            .gnssId     = 0x02,
            .resTrkCh   = 4,
            .maxTrkCh   = 8,
            ._          = 0,
            .flags      = (0x01<<16) | (use.all ? 0x1 : 0x0),
        },
        .beidoo    = {
            .gnssId     = 0x03,
            .resTrkCh   = 8,
            .maxTrkCh   = 16,
            ._          = 0,
            .flags      = (0x01<<16) | (use.all ? 0x1 : 0x0),
        },
        .imes    = {
            .gnssId     = 0x04,
            .resTrkCh   = 4,
            .maxTrkCh   = 8,
            ._          = 0,
            .flags      = (0x01<<16) | (use.all ? 0x1 : 0x0),
        },
        .qzss    = {
            .gnssId     = 0x05,
            .resTrkCh   = 4,
            .maxTrkCh   = 8,
            ._          = 0,
            .flags      = (0x01<<16) | (use.all ? 0x1 : 0x0),
        },
        .glon    = {
            .gnssId     = 0x06,
            .resTrkCh   = 8,
            .maxTrkCh   = 32,//static_cast<uint8_t>(use.gps ? 16 : 32),
            ._          = 0,
            .flags      = (0x01<<16) | (use.glo ? 0x1 : 0x0),
        },
    };
    
    if (!gps.send(UBX_CFG, UBX_CFG_GNSS, data))
        return false;

#ifdef FWVER_DEBUG
    delay(3000);
    CONSOLE("get gnss");
    return gps.get(UBX_CFG, UBX_CFG_GNSS, &gpsRecvGnss);
#else
    return true;
#endif // FWVER_DEBUG
}

bool gpsUpdateCfgModel() {
    CONSOLE("update model");
        
    struct {
        uint16_t mask;              // Only masked parameters will be applied
        uint8_t  dynModel;          // Dynamic platform model
        uint8_t  fixMode;           // Position fixing mode
        int32_t  fixedAlt;          // Fixed altitude (MSL) for 2D mode       (m)
        uint32_t fixedAltVar;       // Fixed altitude variance for 2D mode    (m^2)
        int8_t   minElev;           // Minimum elevation for satellite        (deg)
        uint8_t  drLimit;           // Maximum time to perform dead reckoning (s)
        uint16_t pDop;              // Position DOP mask
        uint16_t tDop;              // Time DOP mask
        uint16_t pAcc;              // Position accuracy mask                 (m)
        uint16_t tAcc;              // Time accuracy mask                     (m)
        uint8_t  staticHoldThresh;  // Static hold threshold                  (cm/s)
        uint8_t  res1;              // Reserved, set to 0
        uint32_t res2;              // Reserved, set to 0
        uint32_t res3;              // Reserved, set to 0
        uint32_t res4;              // Reserved, set to 0
    } cfg_nav5 = {
        .mask       = 0x01 | 0x04,  // Apply dynamic model settings
    };

    cfg_nav5.dynModel =
                cfg.d().navmodel & 0x01 ?
                    (cfg.d().navmodel >> 1) & 0x0f :
                    NAV_MODEL_AIRBORN_2G;
    
    cfg_nav5.fixMode = (cfg.d().navmodel >> 6) & 0x03;
    if (cfg_nav5.fixMode == 0)
        cfg_nav5.fixMode = 0x03;
    
    if (!gps.send(UBX_CFG, UBX_CFG_NAV5, cfg_nav5))
        return false;

#ifdef FWVER_DEBUG
    CONSOLE("get model");
    return gps.get(UBX_CFG, UBX_CFG_NAV5, &gpsRecvModel);
#else
    return true;
#endif // FWVER_DEBUG
}

/* ------------------------------------------------------------------------------------------- *
 *  режим DPS-Direct
 * ------------------------------------------------------------------------------------------- */
void gpsDirectTgl() {
    direct = !direct;

#ifndef FWVER_DEBUG
    if (direct)
        Serial.begin(115200);
    else
        Serial.end();
#endif // not FWVER_DEBUG
}
bool gpsDirect() { return direct; }


/* ------------------------------------------------------------------------------------------- *
 *  Калькуляция
 * ------------------------------------------------------------------------------------------- */
double gpsDistance(double lat1, double long1, double lat2, double long2) {
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  
  return delta * 6372795;
}

double gpsCourse(double lat1, double long1, double lat2, double long2) {
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  
  return degrees(a2);
}


/* ------------------------------------------------------------------------------------------- *
 *  Питание на NAV-модуле
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR uint8_t gpspwr = NAV_PWRBY_PWRON;
bool gpsPwr(uint8_t by) {
    return (gpspwr & by) > 0;
}

void gpsOn(uint8_t by) {
    gpspwr |= by;
    if (gpspwr == 0)
        return;
    
#if HWVER > 1
    // При перезагрузке state сбрасывается в NAV_STATE_OFF, а gpspwr в NAV_PWRBY_PWRON.
    // И, казалось бы, надо включить, но пин остаётся LOW.
    // И если мы будем проверять только пин, то будем ложно считать, что gps у нас включен и проинициализирован
    if ((digitalRead(NAV_PIN_POWER) == LOW) && (state == NAV_STATE_OK))
        return;
    digitalWrite(NAV_PIN_POWER, LOW);
    pinMode(NAV_PIN_POWER, OUTPUT);
#endif
    
    state = NAV_STATE_OK;
    
    naviInit();
}

void gpsPwrDown() {
#if HWVER > 1
    if ((digitalRead(NAV_PIN_POWER) == HIGH) && (state == NAV_STATE_OFF))
        return;
    digitalWrite(NAV_PIN_POWER, HIGH);
    pinMode(NAV_PIN_POWER, OUTPUT);
#endif
    
    state = NAV_STATE_OFF;
}
void gpsOff(uint8_t by) {
    gpspwr &= ~by;
    
    if (gpspwr > 0)
        return;

    gpsPwrDown();
    
    ss.begin(9600);
}

void gpsRestart() {
    digitalWrite(NAV_PIN_POWER, HIGH);
    pinMode(NAV_PIN_POWER, OUTPUT);
    
    delay(1000);

    digitalWrite(NAV_PIN_POWER, LOW);
    gpspwr |= NAV_PWRBY_HAND;
    state = NAV_STATE_OK;
}

void gpsPwrTgl() {
    if (gpspwr)
        gpsOff();
    else
        gpsOn(NAV_PWRBY_HAND);
}
void gpsRestore() {
#if HWVER > 1
    if ((digitalRead(NAV_PIN_POWER) == LOW) && (state == NAV_STATE_OFF)) {
        // при перезагрузке state вбрасывается в NAV_STATE_OFF, но пин при этом
        // не гасится.
        // И у нас нет достоверной информации, проинициализирован gps или нет.
        // В этом случае отключаем питание принудительно
        digitalWrite(NAV_PIN_POWER, HIGH);
        delay(100);
    }
#endif
    
    if (gpspwr > 0)
        gpsOn(0);
    else
        gpsOff();
}


