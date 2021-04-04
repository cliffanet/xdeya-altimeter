/*
    GPS
*/

#ifndef _gps_H
#define _gps_H

#include <Arduino.h>
#include <TinyGPS++.h>

TinyGPSPlus &gpsGet();
void gpsInit();
void gpsProcess();
void gpsDirect();

#endif // _gps_H
