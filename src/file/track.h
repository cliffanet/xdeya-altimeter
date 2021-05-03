/*
    Save Track to file
*/

#ifndef _file_track_H
#define _file_track_H

#include <Arduino.h>
#include "../cfg/jump.h"
#include "../cfg/main.h"

#define TRK_FILE_NAME       "track"
#define TRK_PRESERV_COUNT   32
#define ALL_SIZE_MAX        786432

typedef enum {
    TRKRUN_NONE = 0,
    TRKRUN_FORCE,
    TRKRUN_AUTO
} trk_running_t;

// Заголовок трека
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = CFG_MGC1;                // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = CFG_MAIN_VER;
    uint16_t __ = 0;                        // резерв
    uint32_t id = 0;                        // пока не ведётся, но нужно запоминать ID в отдельном файле и делать ему inc
                                            // при каждом новом треке,
                                            // чтобы при синхронизации понимать где последний отправленный на веб
    uint32_t flags = 0;                     // флаги - пока не используются
    uint32_t jmpnum;                        // текущий номер прыга на момент старта записи
    uint32_t utsbeg;                        // дата/время старта записи трека
    
    uint8_t _[7] = { 0 };                   // резерв
    uint8_t mgc2 = CFG_MGC2;
} trk_head_t;

bool trkStart(bool force = true);
size_t trkStop();
bool trkRunning();
trk_running_t trkState();

int trkFileCount();
size_t trkCountAvail();

void trkProcess();

#endif // _file_track_H
