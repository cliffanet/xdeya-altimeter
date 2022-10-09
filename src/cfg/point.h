/*
    Cfg Navi point
*/

#ifndef _cfg_point_H
#define _cfg_point_H

#include "main.h"

/* 
 *  Конфиг, сохраняемый в SPIFFS
 */

#define PNT_COUNT     3

#define CFG_POINT_ID    5
#define CFG_POINT_VER   1
#define CFG_POINT_NAME  "pnt"

typedef struct __attribute__((__packed__)) {
        bool used = false;
        double lat = 0;
        double lng = 0;
    } cfg_point_el_t;

//  Сохранение Navi-координат
typedef struct __attribute__((__packed__)) {
    uint8_t num = 0;                         // текущая выбранная точка
    cfg_point_el_t all[PNT_COUNT];           // координаты трёх точек (sizeof(double) == 8)
} cfg_point_t;

class ConfigPoint : public Config<cfg_point_t> {
    public:
        ConfigPoint();
        uint8_t numInc();
        uint8_t numDec();
        bool numSet(uint8_t _num);
        bool locSet(double lat, double lng);
        bool locClear();
        
        uint8_t num() const { return data.num; }
        bool numValid() const { return (data.num > 0) && (data.num <= PNT_COUNT); }
        cfg_point_el_t cur() {
                if ((data.num > 0) && (data.num <= PNT_COUNT))
                    return data.all[data.num-1];
                cfg_point_el_t nullp;
                return nullp;
            }
};

extern ConfigPoint pnt;


#endif // _cfg_point_H
