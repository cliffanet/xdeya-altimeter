/*
    Save Track to file
*/

#ifndef _file_track_H
#define _file_track_H

#include <Arduino.h>
#include "../cfg/jump.h"

#define TRK_FILE_NAME       "track"
#define TRK_PRESERV_COUNT   32
#define ALL_SIZE_MAX        786432

typedef enum {
    TRKRUN_NONE = 0,
    TRKRUN_FORCE,
    TRKRUN_AUTO
} trk_running_t;

bool trkStart(bool force = true);
size_t trkStop();
bool trkRunning();
trk_running_t trkState();

int trkFileCount();
size_t trkCountAvail();

void trkProcess();

#endif // _file_track_H
