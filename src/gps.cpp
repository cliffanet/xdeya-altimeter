
#include "gps.h"

#include <TinyGPS++.h>
static TinyGPSPlus gps;

// будем использовать стандартный экземпляр класса HardwareSerial, 
// т.к. он и так в системе уже есть и память под него выделена
// Стандартные пины для свободного аппаратного Serial2: 16(rx), 17(tx)
#define ss Serial2


/* ------------------------------------------------------------------------------------------- *
 *  GPS-обработчик
 * ------------------------------------------------------------------------------------------- */

TinyGPSPlus &gpsGet() { return gps; }

void gpsInit() {
    // инициируем uart-порт GPS-приёмника
    ss.begin(9600);
}

void gpsProcess() {
    while (ss.available()) {
        gps.encode( ss.read() );
    }
}
