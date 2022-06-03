
#include "track.h"
#include "proc.h"
#include "../core/workerloc.h"
#include "../navi/proc.h"
#include "../log.h"
#include "../clock.h"

/* ------------------------------------------------------------------------------------------- *
 *  Файл трека
 * ------------------------------------------------------------------------------------------- */
bool FileTrack::chs_t::eqrev(const struct chs_s & cks1, const struct chs_s & cks2) {
    // временная функция инверсного сравнения данных
    //
    // В прошлых версиях упаковка/распаковка структуры FileTrack::chs_t
    // делалась вручную, по отдельности каждое поле,
    // но на стороне сервера оно упаковывается и распаковывается как hex64
    //
    // Позже и тут упаковка/распаковка этой структуры стала делаться как int64,
    // и поля перестали совпадать, ну или совпадут, если они будут
    // располагаться в обратном порядке: sz, csb, csa
    //
    // Чтобы не менять порядок полей на некрасивый, сделаем альтернативную проверку
    // на совпадение.
    
    struct __attribute__((__packed__)) {
        uint16_t    csa;
        uint16_t    csb;
    }
        ck1 = { cks1.csb, cks1.csa },
                            // одну надо перевернуть, а другую нет
                            // ну вот так несимметрично работала старая упаковка
                            // Поэтому эта функция применяется дважды - напрямую и меняя аргументы местами
        ck2 = { cks2.csa, cks2.csb };
    
    uint32_t
        sz1 = *reinterpret_cast<uint32_t *>(&ck1),
        sz2 = *reinterpret_cast<uint32_t *>(&ck2);
        
    return (sz1 == cks2.sz) && (sz2 == cks1.sz);
}

FileTrack::chs_t FileTrack::chksum() {
    if (!fh)
        return { 0, 0, 0 };
    
    size_t sz = sizehead() + sizeitem();
    uint8_t data[sz];
    chs_t cks = { 0, 0, 0 };
    
    cks.sz = fh.size();
    if (cks.sz == 0)
        return { 0, 0, 0 };
    
    if (sz > cks.sz)
        sz = cks.sz;
    
    fh.seek(0, SeekSet);
    if (fh.read(data, sz) != sz)
        return { 0, 0, 0 };

    for (const auto &d : data) {
        cks.csa += d;
        cks.csb += cks.csa;
    }
    
    if (fh.available() == 0)
        return cks;
    
    fh.seek(cks.sz - sz);
    if (fh.read(data, sz) != sz)
        return { 0, 0, 0 };
    
    for (const auto &d : data) {
        cks.csa += d;
        cks.csb += cks.csa;
    }
    
    return cks;
}

FileTrack::chs_t FileTrack::chksum(uint8_t n) {
    if (fh)
        fh.close();
    
    if (!open(n, MODE_READ, true))
        return { 0, 0, 0 };
    
    auto cks = chksum();
    
    fh.close();
    
    return cks;
}

uint8_t FileTrack::findfile(chs_t cks) {
    for (uint8_t n = 1; n < 99; n++) {
        auto cks1 = chksum(n);
        CONSOLE("chksum[%d]: %04x%04x%08x ok:%d", n, cks1.csa, cks1.csb, cks1.sz, cks1 ? 1 : 0);
        if (!cks1)
            return 0;
        if (cks1 == cks)
            return n;
    }
    
    return 0;
}

bool FileTrack::create(const head_t &head) {
    if (!rotate())
        return false;
    
    if (!open(1, MODE_WRITE, true))
        return false;
    
    return add(head);
}

/* ------------------------------------------------------------------------------------------- *
 *  Под сколько ещё записей доступно место
 * ------------------------------------------------------------------------------------------- */
size_t trkCountAvail() {
    size_t used = fileSizeAvail();
    if (used >= ALL_SIZE_MAX)
        return 0;
    return (ALL_SIZE_MAX - used) / FileTrack::sizeitem();
}

/* ------------------------------------------------------------------------------------------- *
 *  Проверка и освобождение места (удаление старых записей)
 * ------------------------------------------------------------------------------------------- */
static bool trkCheckAvail(bool removeFirst) {
    size_t av;
    while ((av = trkCountAvail()) < TRK_PRESERV_COUNT) {
        CONSOLE("avail: %d records", av);
        int cnt = FileTrack().count();
        if (cnt <= 0) {
            CONSOLE("track nothing to remove");
            return false;
        }
        if ((cnt == 1) && !removeFirst) {
            CONSOLE("remained only 1 track");
            return false;
        }
        if (!FileTrack().remove(cnt)) {
            CONSOLE("can't remove track[%d]", cnt);
            return false;
        }
        CONSOLE("track[%d] removed ok", cnt);
    }
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Процесс трекинга
 * ------------------------------------------------------------------------------------------- */
class WorkerTrkSave : public WorkerProc
{
    private:
        uint8_t m_by;
        uint8_t m_cnt;
        uint32_t tmoffset;
        jmp_cur_t prelogcur;
        bool useext;
        FileTrack tr;
        
    public:
        WorkerTrkSave(uint8_t by, uint16_t old = 0) : m_by(by), m_cnt(0) {
            prelogcur = jmpPreCursor()-old;
            tmoffset = 0;
            if (old > 0)
                // если мы начали не с текущего момента, то отнимем из offset первую запись,
                // в этом случае лог будет красиво начинаться с tmoffset=0.
                // Ну а когда мы начали с текущего момента, то мы не можем залезть в будущее и узнать,
                // какой там будет tmoffset, поэтому в этом случае начинаться лог будет уже не с нуля.
                // Но это и логично, т.к. первая запись в логе появится не сразу после запуска,
                // а только с появлением её в prelog
                tmoffset -= jmpPreLog(prelogcur+1).tmoffset;
        }
        void byadd(uint8_t by) { m_by |= by; }
        void bydel(uint8_t by) { m_by &= ~by; }
        
        void begin() {
            if (cfg.d().gpsontrkrec)
                gpsOn(GPS_PWRBY_TRKREC);
            useext = fileExtInit();
            CONSOLE("track started by: 0x%02x, useext: %d", m_by, useext);
        }
        
        void end() {
            jmp.keyreset();
    
            if (cfg.d().gpsontrkrec)
                gpsOff(GPS_PWRBY_TRKREC);
            if (useext)
                fileExtStop();
            CONSOLE("track stopped");
        }
        
        state_t process() {
            if (m_by == 0)
                return STATE_END;
            
            if (!tr) {
                // пишем заголовок - время старта и номер прыга
                FileTrack::head_t th;
                th.jmpnum = jmp.count();
                if (jmp.state() == LOGJMP_NONE) // в случае, если прыг не начался (включение трека до начала прыга),
                    th.jmpnum ++;          // за номер прыга считаем следующий
                th.jmpkey = jmp.key();
                th.tmbeg = tmNow(jmpPreInterval(prelogcur));
                
                return
                   tr.create(th) ? STATE_RUN : STATE_END;
            }
            
            //CONSOLE("diff: %d", jmpPreCursor()-prelogcur);
            if (prelogcur == jmpPreCursor())
                return STATE_WAIT;
            
            prelogcur++;
            auto ti = jmpPreLog(prelogcur);
            
            tmoffset += ti.tmoffset;
            ti.tmoffset = tmoffset;
    
            ti.msave = utm() / 1000;
    
            if (!tr.add(ti)) {
                CONSOLE("track save fail");
                return STATE_END;
            }
            
            //CONSOLE("save[%d]: %d", *prelogcur, tmoffset);
            
            if (useext)
                return STATE_RUN;
            
            if ((((m_cnt++) & 0b111) == 0) && !trkCheckAvail(false))
                return STATE_END;
            
            return STATE_RUN;
        }
};

static WorkerTrkSave * trkProc() {
    return
        reinterpret_cast<WorkerTrkSave *>(
            wrkGet(WORKER_TRK_SAVE)
        );
}


/* ------------------------------------------------------------------------------------------- *
 *  Запуск трекинга
 * ------------------------------------------------------------------------------------------- */
bool trkStart(uint8_t by, uint16_t old) {
    if (by == 0)
        return false;
    
    auto proc = trkProc();
    if (proc != NULL) {
        proc->byadd(by);
        return true;
    }
    
    wrkAdd(WORKER_TRK_SAVE, new WorkerTrkSave(by, old));
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Остановка
 * ------------------------------------------------------------------------------------------- */
void trkStop(uint8_t by) {
    if (by == 0)
        return;
    
    auto proc = trkProc();
    if (proc != NULL)
        proc->bydel(by);
}

/* ------------------------------------------------------------------------------------------- *
 *  Текущее состояние
 * ------------------------------------------------------------------------------------------- */
bool trkRunning(uint8_t by) {
    return wrkExists(WORKER_TRK_SAVE);
}



/* ------------------------------------------------------------------------------------------- *
 *  Отправка треков на сервер
 * ------------------------------------------------------------------------------------------- */
/*
class WorkerTrkSend : public WorkerProc
{
    private:
        FileTrack tr;
        NetSocket *m_nsock;
    
    public:
        WorkerTrkSend(NetSocket *nsock) : m_nsock(nsock) {
        }
        
    state_t process() {
        // проверка подключения к серверу
        if ((m_sock != NULL) && (!m_sock->connected()))
            RETURN_ERR(SrvDisconnect);
        
        return STATE_END;
    }
};
*/

uint8_t trkSenderCreate(NetSocket *nsock) {
    if (nsock == NULL)
        return 0;
    
    //return wrkAddRand(new WorkerTrkSend(nsock));
    return 0;
}
