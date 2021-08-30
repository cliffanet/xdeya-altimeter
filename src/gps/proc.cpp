
#include "proc.h"
#include "../log.h"
#include "../clock.h"

// будем использовать стандартный экземпляр класса HardwareSerial, 
// т.к. он и так в системе уже есть и память под него выделена
// Стандартные пины для свободного аппаратного Serial2: 16(rx), 17(tx)
#include <Arduino.h> // Serial, delay
#define ss Serial2

#include "ubloxproto.h"
static UbloxGpsProto gps(ss);

static gps_data_t data = { 0 };

static bool direct = false;

/* ------------------------------------------------------------------------------------------- *
 *  GPS-получение данных
 * ------------------------------------------------------------------------------------------- */
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

static void gpsRecvPosllh(UbloxGpsProto &gps) {
    struct {
    	uint32_t iTOW;     // GPS time of week             (ms)
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
    	uint32_t iTOW;     // GPS time of week             (ms)
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
    	uint32_t iTOW;     // GPS time of week             (ms)
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
    	uint32_t iTOW;     // GPS time of week             (ms)
    	int32_t  fTOW;     // Fractional nanoseconds       (ns)
    	int16_t  week;     // GPS week
    	uint8_t  gpsFix;   // GPS fix type
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
    	uint32_t iTOW;         // GPS time of week              (ms)
    	uint16_t year;         // Year                          (1999..2099)
    	uint8_t  month;        // Month                         (1..12)
    	uint8_t  day;          // Day of month                  (1..31)
    	uint8_t  hour;         // Hour of day                   (0..23)
    	uint8_t  min;          // Minute of hour                (0..59)
    	uint8_t  sec;          // Second of minute              (0..59)
    	uint8_t  valid;        // Validity flags
    	uint32_t tAcc;         // Time accuracy estimate        (ns)
    	int32_t  nano;         // Nanoseconds of second         (ns)
    	uint8_t  gpsFix;       // GPS fix type
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
 *  Данные о GPS для внешнего использования
 * ------------------------------------------------------------------------------------------- */

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


/* ------------------------------------------------------------------------------------------- *
 *  GPS-инициализация
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

static bool gpsUartSpeed(uint32_t baudRate) {
    // инициируем uart-порт GPS-приёмника
    ss.begin(9600);
    ss.setRxBufferSize(512); // По умолчанию = 256 и этого не хватает, чтобы принять сразу все присылаемые от GPS данные за один цикл
    
    char cnt = 0;
    
    while (cnt < 5) {
        cnt++;
        
        CONSOLE("Set UART(gps) default speed 9600 [try #%d]", cnt);
        ss.updateBaudRate(9600);
        
        struct
        {
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
    		.baudRate     = baudRate,  // Baudrate in bits/second
    		.inProtoMask  = 0x0001, // UBX protocol
    		.outProtoMask = 0x0001, // UBX protocol
    		.flags        = 0,      // Flags bit mask
    		.reserved5    = 0       // Reserved, set to 0
    	};
        
        if (!gps.send(UBX_CFG, UBX_CFG_PRT, cfg_prt))
            return false;
        
        delay(10);
        
        gps.cnfclear();
        gps.rcvclear();
        ss.flush();
        while (ss.available()) ss.read();
        
        CONSOLE("Set UART(gps) speed %d", baudRate);
        ss.updateBaudRate(baudRate);
        delay(10);
        
        if (!gps.send(UBX_CFG, UBX_CFG_PRT, cfg_prt))
            return false;
        
        if (gpsCmdConfirm())
            return true;
    }
    
    return false;
}

static bool gpsInitCmd() {
    if (!gpsUartSpeed(34800)) {
        CONSOLE("Can't set GPS-UART baudRate");
        return false;
    }
    CONSOLE("GPS-UART baudRate init ok");
    
    gps.hndclear();
    gps.hndadd(UBX_NAV,  UBX_NAV_POSLLH,     gpsRecvPosllh);
    gps.hndadd(UBX_NAV,  UBX_NAV_VELNED,     gpsRecvVelned);
    gps.hndadd(UBX_NAV,  UBX_NAV_TIMEUTC,    gpsRecvTimeUtc);
    gps.hndadd(UBX_NAV,  UBX_NAV_SOL,        gpsRecvSol);
    gps.hndadd(UBX_NAV,  UBX_NAV_PVT,        gpsRecvPvt);
    
    struct {
    	uint8_t msgClass;  // Message class
    	uint8_t msgID;     // Message identifier
    	uint8_t rate;      // Send rate
    } cfg_rate[] = {
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
    
    for (auto r : cfg_rate)
        if (!gps.send(UBX_CFG, UBX_CFG_MSG, r) || !gpsCmdConfirm())
            return false;

    struct {
    	uint16_t measRate;  // Measurement rate             (ms)
    	uint16_t navRate;   // Nagivation rate, in number 
    	                       //   of measurement cycles
    	uint16_t timeRef;   // Alignment to reference time:
    	                       //   0 = UTC time; 1 = GPS time
    } cfg_mrate = {
		.measRate   = 200,      // Measurement rate (ms)
		.navRate    = 1,        // Navigation rate (cycles)
		.timeRef    = 0         // UTC time
	};
    if (!gps.send(UBX_CFG, UBX_CFG_RATE, cfg_mrate) || !gpsCmdConfirm())
        return false;
	
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
		.mask       = 0x0001,       // Apply dynamic model settings
		.dynModel   = 7             // Airborne with < 1 g acceleration
	};
    if (!gps.send(UBX_CFG, UBX_CFG_NAV5, cfg_nav5) || !gpsCmdConfirm())
        return false;
    
    struct {
    	uint16_t navBbrMask; // BBR sections to clear
    	uint8_t  resetMode;  // Reset type
    	uint8_t  res;        // Reserved
    } cfg_rst =	{
		.navBbrMask = 0x0000,   // Hot start
		.resetMode  = 0x09      // Controlled GPS start
	};
    if (!gps.send(UBX_CFG, UBX_CFG_RST, cfg_rst) || !gpsCmdConfirm())
        return false;
    
    CONSOLE("GPS-UART config ok");
    
    return true;
}

void gpsInit() {
    if (!gpsInitCmd()) {
        CONSOLE("GPS init fail");
        gpsFree();
    }
}

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
}

/* ------------------------------------------------------------------------------------------- *
 *  режим DPS-Direct
 * ------------------------------------------------------------------------------------------- */
void gpsDirectTgl() {
    direct = !direct;
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
