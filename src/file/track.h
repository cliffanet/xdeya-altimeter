/*
    Save Track to file
*/

#ifndef _file_track_H
#define _file_track_H

#include "../../def.h"

#include "../cfg/jump.h"
#include "../cfg/main.h"
#include "../core/filebin.h"
#include "../clock.h"

#define TRK_FILE_NAME       "track"
#define TRK_PRESERV_COUNT   32
#define ALL_SIZE_MAX        786432

class FileTrack : public FileBinNum {
    public:
        // Контрольная сумма трека
        typedef struct  __attribute__((__packed__)) chs_s {
            uint16_t    csa;
            uint16_t    csb;
            uint32_t    sz;
    
            bool operator== (const struct chs_s & cks) {
                return (this == &cks) || ((this->csa == cks.csa) && (this->csb == cks.csb) && (this->sz == cks.sz));
            };
            operator bool() { return (csa != 0) && (csb != 0) && (sz != 0); }
        } chs_t;
        
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
        } head_t;
        
        typedef log_item_t item_t;
        
        FileTrack() : FileBinNum(PSTR(TRK_FILE_NAME)) {}
        FileTrack(uint8_t n, mode_t mode = MODE_READ) : FileTrack() { open(n, mode, true); }

        static inline size_t sizehead() { return sizeof(head_t)+4; }
        static inline size_t sizeitem() { return sizeof(item_t)+4; }
        size_t pos() const { return (fh.position() - sizehead()) / sizeitem(); }
        size_t avail() { return (fh.available() - sizehead()) / sizeitem(); }
        size_t sizefile() const { return (fh.size() - sizehead()) / sizeitem(); }
        
        chs_t chksum();
        chs_t chksum(uint8_t n);
        uint8_t findfile(chs_t cks);

        virtual size_t count() { return FileBinNum::count(true); }
        virtual bool renum() { return FileBinNum::renum(true); }
        virtual bool rotate(uint8_t count = 0) { return FileBinNum::rotate(count, true); }
        virtual bool remove(uint8_t n) { return FileBinNum::remove(n, true); }
        
        bool create(const head_t &head);
};

// Под сколько ещё записей доступно место
size_t trkCountAvail();



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

#endif // _file_track_H
