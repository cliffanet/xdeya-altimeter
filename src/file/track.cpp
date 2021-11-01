
#include "track.h"
#include "../jump/proc.h"
#include "../gps/proc.h"
#include "log.h"
#include "../log.h"
#include "../clock.h"

static uint8_t state = 0;
static bool runned = false;
static uint32_t tmoffset = 0;
static uint16_t prelogcur = 0;
File fh;

/* ------------------------------------------------------------------------------------------- *
 *  Запуск трекинга
 * ------------------------------------------------------------------------------------------- */
static bool trkCheckAvail(bool removeFirst = false);
bool trkStart(uint8_t by, uint16_t old) {
    state |= by;
    if (state == 0)
        return false;
    
    if (runned)
        return true;
    
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
    viewIntDis();
    fh = DISKFS.open(fname, FILE_WRITE);
    if (!fh) {
        viewIntEn();
        return false;
    }
    
    // пишем заголовок - время старта и номер прыга
    trk_head_t th;
    th.jmpnum = jmp.count();
    if (jmp.state() == LOGJMP_NONE) // в случае, если прыг не начался (включение трека до начала прыга),
        th.jmpnum ++;          // за номер прыга считаем следующий
    th.jmpkey = jmp.key();
    th.tmbeg = tmNow(jmpPreLogInterval(old));
    
    if (!fwrite(fh, th)) {
        fh.close();
        viewIntEn();
        return false;
    }
    
    // сбрасываем tmoffset, чтобы у самой первой записи он был равен нулю
    tmoffset = 0;
    tmoffset -= jmpPreLog(old).tmoffset;
    
    // Скидываем сразу все презапомненные данные
    // даже если old == 0, мы всё равно запишем текущую позицию
    while (1) {
        log_item_t log;
        log = jmpPreLog(old);
        tmoffset += log.tmoffset;
        log.tmoffset = tmoffset;
        
        log.msave = utm() / 1000;
    
        if (!fwrite(fh, log)) {
            fh.close();
            viewIntEn();
            return false;
        }
        
        if (old == 0)
            break;
        old--;
    }
    viewIntEn();
    
    prelogcur = jmpPreLogFirst();
    runned = true;
    CONSOLE("track started by 0x%02x", by);
    
    if (cfg.d().gpsontrkrec)
        gpsOn(GPS_PWRBY_TRKREC);
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Остановка
 * ------------------------------------------------------------------------------------------- */
void trkStop(uint8_t by) {
    state &= ~by;
    if ((state > 0) || !runned)
        return;
    
    fh.close();
    
    runned = false;
    jmp.keyreset();
    CONSOLE("track stopped");
    
    if (cfg.d().gpsontrkrec)
        gpsOff(GPS_PWRBY_TRKREC);
}

/* ------------------------------------------------------------------------------------------- *
 *  Текущее состояние
 * ------------------------------------------------------------------------------------------- */
bool trkRunning(uint8_t by) {
    return (state & by) > 0;
}

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
    return (ALL_SIZE_MAX - used) / LOG_REC_SIZE(sizeof(log_item_t));
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
    if (state == 0)
        return;
    
    if (!fh)
        return;
    
    static uint8_t cnt = 0;
    if ((((cnt++) & 0b111) == 0) && !trkCheckAvail()) {
        trkStop();
        return;
    }

    log_item_t log;
    while (jmpPreLogNext(prelogcur, &log)) {
        tmoffset += log.tmoffset;
        log.tmoffset = tmoffset;
        
        log.msave = utm() / 1000;
        
        if (!fwrite(fh, log))
            trkStop();
    }
}
