/*
    GPS - processing
*/

#ifndef _gps_H
#define _gps_H

#include <stdint.h>

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


typedef struct {
	int32_t  nano;     // Nanoseconds of second        (ns)
	uint16_t year;     // Year                         (1999..2099)
	uint8_t  mon;      // Month                        (1..12)
	uint8_t  day;      // Day of month                 (1..31)
	uint8_t  h;        // Hour of day                  (0..23)
	uint8_t  m;        // Minute of hour               (0..59)
	uint8_t  s;        // Second of minute             (0..59)
} gps_tm_t;

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
    
    gps_tm_t tm;
    bool rcvok;
} gps_data_t;

const gps_data_t &gpsInf();
uint32_t gpsRecv();
uint32_t gpsRecvError();
uint32_t gpsCmdUnknown();
uint32_t gpsDataAge();
void gpsInit();
void gpsProcess();

void gpsDirectTgl();
bool gpsDirect();

double gpsDistance(double lat1, double long1, double lat2, double long2);
double gpsCourse(double lat1, double long1, double lat2, double long2);

#endif // _gps_H
