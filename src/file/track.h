/*
    Save Track to file
*/

#ifndef _file_track_H
#define _file_track_H

#include "../../def.h"

#include "../cfg/jump.h"
#include "../cfg/main.h"
#include "../clock.h"

#define TRK_FILE_NAME       "track"
#define TRK_PRESERV_COUNT   32
#define ALL_SIZE_MAX        786432

// Заголовок трека
typedef struct __attribute__((__packed__)) {
    uint32_t id = 0;                        // пока не ведётся, но нужно запоминать ID в отдельном файле и делать ему inc
                                            // при каждом новом треке,
                                            // чтобы при синхронизации понимать где последний отправленный на веб
    uint32_t flags = 0;                     // флаги - пока не используются
    uint32_t jmpnum;                        // текущий номер прыга на момент старта записи
    uint32_t jmpkey;                        // ключ прыга на момент старта записи
    tm_t     tmbeg;                         // дата/время старта записи трека
    
    uint8_t _[8] = { 0 };                   // резерв
} trk_head_t;

#define TRK_RUNBY_HAND      0x01
#define TRK_RUNBY_JMPBEG    0x02
#define TRK_RUNBY_ALT       0x08
#define TRK_RUNBY_ANY       0xFF


bool trkStart(uint8_t by = TRK_RUNBY_HAND, uint16_t old = 0);
void trkStop(uint8_t by = TRK_RUNBY_ANY);
bool trkRunning(uint8_t by = TRK_RUNBY_ANY);

int trkFileCount();
size_t trkCountAvail();

void trkProcess();

#endif // _file_track_H
