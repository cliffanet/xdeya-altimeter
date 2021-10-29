/*
    GPS - processing
*/

#ifndef _gps_H
#define _gps_H

#include "../../def.h"
#include "../clock.h"

#define GPS_PIN_POWER     27

#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399

#define GPS_KOEF_LATLON 10000000L
#define GPS_KOEF_DEG    100000L
#define GPS_KOEF_MM     1000L
#define GPS_KOEF_CM     100L

#define GPS_LATLON(x)   ((double)(x) / GPS_KOEF_LATLON)
#define GPS_DEG(x)      ((double)(x) / GPS_KOEF_DEG)
#define GPS_MM(x)       ((double)(x) / GPS_KOEF_MM)
#define GPS_CM(x)       ((double)(x) / GPS_KOEF_CM)

#define GPS_VALID(gps)              (gps.rcvok && (gps.numSV > 0) && (gps.gpsFix == 3))
#define GPS_VALID_LOCATION(gps)     (GPS_VALID(gps) && (gps.hAcc < 30000))
#define GPS_VALID_VERTICAL(gps)     (GPS_VALID(gps) && (gps.vAcc < 30000))
#define GPS_VALID_SPEED(gps)        (GPS_VALID_LOCATION(gps) && (gps.sAcc < 1000))
#define GPS_VALID_HEAD(gps)         (GPS_VALID_LOCATION(gps) && (gps.cAcc < 5000000))
#define GPS_VALID_TIME(gps)         (GPS_VALID(gps) && (gps.tm.year > 2000))

#define GPS_TICK_INTERVAL       200

typedef struct {
	int32_t  lon;      // Longitude                    (deg * 10^7)
	int32_t  lat;      // Latitude                     (deg * 10^7)
	int32_t  hMSL;     // Height above mean sea level  (mm)
	uint32_t hAcc;     // Horizontal accuracy estimate (mm)
	uint32_t vAcc;     // Vertical accuracy estimate   (mm)

	uint8_t  gpsFix;   // GPS fix type
	uint8_t  numSV;    // Number of SVs in solution

	int32_t  velN;     // North velocity               (cm/s)
	int32_t  velE;     // East velocity                (cm/s)
	int32_t  velD;     // Down velocity                (cm/s)
	int32_t  speed;    // 3D speed                     (cm/s)
	int32_t  gSpeed;   // Ground speed                 (cm/s)
	int32_t  heading;  // 2D heading                   (deg * 10^5)
	uint32_t sAcc;     // Speed accuracy estimate      (cm/s)
	uint32_t cAcc;     // Heading accuracy estimate    (deg * 10^5)
    
    tm_t tm;
    bool rcvok;
} gps_data_t;

typedef enum {
    GPS_STATE_OFF = 0,
    GPS_STATE_INIT,
    GPS_STATE_FAIL,
    GPS_STATE_NODATA,
    GPS_STATE_OK
} gps_state_t;

const gps_data_t &gpsInf();
uint32_t gpsRecv();
uint32_t gpsRecvError();
uint32_t gpsCmdUnknown();
uint8_t gpsDataAge();
gps_state_t gpsState();
void gpsInit();
void gpsProcess();

void gpsDirectTgl();
bool gpsDirect();

double gpsDistance(double lat1, double long1, double lat2, double long2);
double gpsCourse(double lat1, double long1, double lat2, double long2);

#define GPS_PWRBY_HAND      0x01
#define GPS_PWRBY_PWRON     0x02
#define GPS_PWRBY_TRKREC    0x04
#define GPS_PWRBY_ALT       0x08
#define GPS_PWRBY_ANY       0xFF

bool gpsPwr(uint8_t by = GPS_PWRBY_ANY);
void gpsOn(uint8_t by = GPS_PWRBY_HAND);
void gpsPwrDown();
void gpsOff(uint8_t by = GPS_PWRBY_ANY);
void gpsRestart();
void gpsPwrTgl();
void gpsRestore();

#endif // _gps_H
