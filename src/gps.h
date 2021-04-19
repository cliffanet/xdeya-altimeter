/*
    GPS
*/

#ifndef _gps_H
#define _gps_H

#include <stddef.h>
#include <TinyGPS++.h>

typedef struct {
	int32_t  lon;      // Longitude                    (deg)
	int32_t  lat;      // Latitude                     (deg)
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
	int32_t  heading;  // 2D heading                   (deg)
	uint32_t sAcc;     // Speed accuracy estimate      (cm/s)
	uint32_t cAcc;     // Heading accuracy estimate    (deg)
    
    struct {
    	int32_t  nano;     // Nanoseconds of second        (ns)
    	uint16_t year;     // Year                         (1999..2099)
    	uint8_t  mon;      // Month                        (1..12)
    	uint8_t  day;      // Day of month                 (1..31)
    	uint8_t  h;        // Hour of day                  (0..23)
    	uint8_t  m;        // Minute of hour               (0..59)
    	uint8_t  s;        // Second of minute             (0..59)
    } tm;
} gps_data_t;

TinyGPSPlus &gpsGet();
const gps_data_t &gpsInf();
void gpsInit();
void gpsProcess();

void gpsDirectTgl();
bool gpsDirect();

#endif // _gps_H
