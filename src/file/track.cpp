
#include "track.h"
#include "../jump/proc.h"
#include "../gps/proc.h"
#include "log.h"
#include "../log.h"
#include "../clock.h"

static trk_running_t state = TRKRUN_NONE;
static uint32_t tmoffset = 0;
static uint16_t prelogcur = 0;
File fh;

/* ------------------------------------------------------------------------------------------- *
 *  Запуск трекинга
 * ------------------------------------------------------------------------------------------- */
static bool trkCheckAvail(bool removeFirst = false);
bool trkStart(bool force, uint16_t old) {
    if (state != TRKRUN_NONE)
        trkStop();
    
    if (!trkCheckAvail(true))
        return false;
    
    // открываем файл
    const auto *_fname = PSTR(TRK_FILE_NAME);
    if (!logRotate(_fname, 0))
        return false;
    
    char fname[36];
    
    fname[0] = '/';
    strncpy_P(fname+1, _fname, 30);
    fname[30] = '\0';
    const byte flen = strlen(fname);
    sprintf_P(fname+flen, PSTR(LOGFILE_SUFFIX), 1);
    fh = DISKFS.open(fname, FILE_WRITE);
    if (!fh)
        return false;
    
    // пишем заголовок - время старта и номер прыга
    struct log_item_s <trk_head_t> th;
    th.data.jmpnum = jmp.count();
    if (jmp.state() == LOGJMP_NONE) // в случае, если прыг не начался (включение трека до начала прыга),
        th.data.jmpnum ++;          // за номер прыга считаем следующий
    th.data.tmbeg = tmNow(jmpPreLogInterval(old));
    
    auto sz = fh.write(reinterpret_cast<const uint8_t *>(&th), sizeof(th));
    if (sz != sizeof(th)) {
        fh.close();
        return false;
    }
    
    // сбрасываем tmoffset, чтобы у самой первой записи он был равен нулю
    tmoffset = 0;
    tmoffset -= jmpPreLog(old).tmoffset;
    
    // Скидываем сразу все презапомненные данные
    // даже если old == 0, мы всё равно запишем текущую позицию
    while (1) {
        struct log_item_s <log_item_t> log;
        log.data = jmpPreLog(old);
        tmoffset += log.data.tmoffset;
        log.data.tmoffset = tmoffset;
        
        log.data.msave = utm() / 1000;
    
        auto sz = fh.write(reinterpret_cast<const uint8_t *>(&log), sizeof(log));
        if (sz != sizeof(log)) {
            fh.close();
            return false;
        }
        
        if (old == 0)
            break;
        old--;
    }
    
    state = force ? TRKRUN_FORCE : TRKRUN_AUTO;
    prelogcur = jmpPreLogFirst();
    CONSOLE("track started %s", force ? "force" : "auto");
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Остановка
 * ------------------------------------------------------------------------------------------- */
size_t trkStop() {
    if (state == TRKRUN_NONE)
        return -1;
    
    fh.close();
    
    state = TRKRUN_NONE;
    CONSOLE("track stopped");
    return 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  Текущее состояние
 * ------------------------------------------------------------------------------------------- */
bool trkRunning() { return state > TRKRUN_NONE; }
trk_running_t trkState() { return state; }

/* ------------------------------------------------------------------------------------------- *
 *  Сколько файлов есть в наличии
 * ------------------------------------------------------------------------------------------- */
int trkFileCount() {
    return logCount(PSTR(TRK_FILE_NAME));
}

/* ------------------------------------------------------------------------------------------- *
 *  Под сколько ещё записей доступно место
 * ------------------------------------------------------------------------------------------- */
size_t trkCountAvail() {
    size_t used = DISKFS.usedBytes();
    if (used >= ALL_SIZE_MAX)
        return 0;
    return (ALL_SIZE_MAX - used) / sizeof(struct log_item_s <log_item_t>);
}

/* ------------------------------------------------------------------------------------------- *
 *  Проверка и освобождение места (удаление старых записей)
 * ------------------------------------------------------------------------------------------- */
static bool trkCheckAvail(bool removeFirst) {
    auto av = trkCountAvail();
    CONSOLE("avail: %d records", av);
    while (av < TRK_PRESERV_COUNT) {
        int avail = logRemoveLast(PSTR(TRK_FILE_NAME), removeFirst);
        if (avail <= 0) {
            CONSOLE("track nothing to remove");
            return false;
        }
        av = trkCountAvail();
    }
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Запись (на каждый тик тут приходится два тика высотомера и один тик жпс)
 * ------------------------------------------------------------------------------------------- */
void trkProcess() {
    if (state == TRKRUN_NONE)
        return;
    
    if (!fh)
        return;
    
    static uint8_t cnt = 0;
    if ((((cnt++) & 0b111) == 0) && !trkCheckAvail()) {
        trkStop();
        return;
    }

    struct log_item_s <log_item_t> log;
    while (jmpPreLogNext(prelogcur, &(log.data))) {
        tmoffset += log.data.tmoffset;
        log.data.tmoffset = tmoffset;
        
        log.data.msave = utm() / 1000;
        
        auto sz = fh.write(reinterpret_cast<const uint8_t *>(&log), sizeof(log));
        if (sz != sizeof(log))
            trkStop();
    }
}
