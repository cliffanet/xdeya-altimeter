/*
    Bluetooth functions
*/

#ifndef _net_bluetooth_H
#define _net_bluetooth_H

#include "../../def.h"

#ifdef USE_BLUETOOTH


// Имена btStart/btStop заняты в cores/esp32/esp32-hal-bt.c

bool bluetoothStart();
bool bluetoothStop();
#endif // #ifdef USE_BLUETOOTH

#endif // _net_bluetooth_H
