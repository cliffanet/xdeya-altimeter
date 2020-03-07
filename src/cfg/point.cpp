/*
    Cfg GPS point
*/

#include "point.h"

ConfigPoint pnt;

/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса ConfigPoint
 * ------------------------------------------------------------------------------------------- */

ConfigPoint::ConfigPoint() :
    Config(PSTR(CFG_POINT_NAME), CFG_POINT_VER)
{
    
}

uint8_t ConfigPoint::numInc() {
    if (data.num < PNT_COUNT)
        data.num++;
    else
        data.num = 0;
    
    _modifed = true;
    return data.num;
}

uint8_t ConfigPoint::numDec() {
    if (data.num > 0)
        data.num--;
    else
        data.num = PNT_COUNT;
    
    _modifed = true;
    return data.num;
}

bool ConfigPoint::numSet(uint8_t _num) {
    if ((_num < 1) || (_num > PNT_COUNT))
        return false;
    data.num = _num-1;
    
    _modifed = true;
    return true;
}

bool ConfigPoint::locSet(double lat, double lng) {
    if ((data.num < 1) || (data.num > PNT_COUNT))
        return false;
    
    auto &p = data.all[data.num-1];
    p.used = true;
    p.lat = lat;
    p.lng = lng;
    
    _modifed = true;
    return true;
}

bool ConfigPoint::locClear() {
    if ((data.num < 1) || (data.num > PNT_COUNT))
        return false;
    
    auto &p = data.all[data.num-1];
    p.used = false;
    p.lat = 0;
    p.lng = 0;
    
    _modifed = true;
    return true;
}

