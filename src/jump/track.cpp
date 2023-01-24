
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
    
    // созраняем позицию
    size_t pos = fh.position();
    
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
    
    // восстанавливаем позицию в файле
    fh.seek(pos, SeekSet);
    
    return cks;
}

FileTrack::chs_t FileTrack::chksum(uint8_t n) {
    if (fh)
        fh.close();
    
    if (!open(n, MODE_READ))
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

bool FileTrack::create(const head_t &head, bool _rotate) {
    if (_rotate && !rotate())
        return false;
    
    if (!open(1, MODE_WRITE))
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
bool file_exists(const char *fname, bool external);
bool file_rename(const char *fname1, const char *fname2, bool external);
bool file_remove(const char *fname, bool external);

#define WRK_RETURN_ERR(s, ...)  do { CONSOLE(s, ##__VA_ARGS__); WRK_RETURN_END; } while (0)
#define WPRC_ERR(s, ...)  do { CONSOLE(s, ##__VA_ARGS__); return END; } while (0)

WRK_DEFINE(TRK_ROTATE) {
    private:
        bool m_isok;
        bool m_useext;
        uint8_t m_fn;
        const char *m_fname_P;
    
    public:
        bool isok() const { return m_isok; }
    
    WRK_CLASS(TRK_ROTATE)(bool external, bool noremove = false) :
        m_isok(false),
        m_useext(external),
        m_fn(1),
        m_fname_P(PSTR(TRK_FILE_NAME))
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        CONSOLE("track rotate begin, useext: %d", m_useext);
    
    WRK_BREAK
        // ищем первый свободный слот
        char fname[37];
        fileName(fname, sizeof(fname), m_fname_P, m_fn);
        if (file_exists(fname, m_useext)) {
            if (m_fn < 99) {
                m_fn++; // пока ещё не достигли максимального номера файла, продолжаем
                WRK_RETURN_RUN;
            }
            
            // достигнут максимальный номер файла
            // его надо удалить и посчитать первым свободным слотом
            if (!file_remove(fname, m_useext))
                WRK_RETURN_ERR("Can't remove file '%s'", fname);
            CONSOLE("file '%s' removed", fname);
        }
        CONSOLE("free fnum: %d", m_fn);
    
    WRK_BREAK_RUN
        // теперь переименовываем все по порядку
        if (m_fn > 1) {
            char src[37], dst[37];
            fileName(src, sizeof(src), m_fname_P, m_fn-1);
            fileName(dst, sizeof(dst), m_fname_P, m_fn);
        
            if (!file_rename(src, dst, m_useext))
                WRK_RETURN_ERR("Can't rename file '%s' to '%s'", src, dst);
            CONSOLE("file '%s' renamed to '%s'", src, dst);
        
            m_fn--;
            WRK_RETURN_RUN;
        }

        CONSOLE("track rotate finish");
        m_isok = true;
    WRK_END
};

WrkProc::key_t trkRotate(bool external, bool noremove) {
    if (!noremove)
        return wrkRand(TRK_ROTATE, external, noremove);
    
    wrkRun(TRK_ROTATE, external, noremove);
    return WRKKEY_TRK_ROTATE;
}

/* ------------------------------------------------------------------------------------------- *
 *  Процесс трекинга
 * ------------------------------------------------------------------------------------------- */
class trkWrk : public Wrk2 {
    uint8_t m_by;
    uint8_t m_cnt;
    uint32_t tmoffset;
    jmp_cur_t prelogcur;
    bool useext;
    FileTrack tr;

public:
    trkWrk(uint8_t by, uint16_t old = 0) : m_by(by), m_cnt(0) {
        CONSOLE("track(0x%08x) begin by:0x%02x", this, by);
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
    ~trkWrk() {
        CONSOLE("track(0x%08x) destroy", this);
    }

    uint8_t by() const { return m_by; };
    void byadd(uint8_t by) { m_by |= by; }
    void bydel(uint8_t by) { m_by &= ~by; }
    
    state_t run() {

    WPROC
        if (cfg.d().navontrkrec)
            gpsOn(NAV_PWRBY_TRKREC);
#ifdef USE_SDCARD
        useext = fileExtInit();
#else
        useext = false;
#endif
        CONSOLE("track starting by: 0x%02x, useext: %d", m_by, useext);
        
        // rotate файлов
        trkRotate(useext, true);
        
    WPRC_RUN
        // Проверяем завершение выполнения rotate
        const auto wrk = wrkGet(TRK_ROTATE);
        if (wrk == NULL)
            WPRC_ERR("Rotate die");
        if (wrk->isrun()) {
            CONSOLE("Wait rotate");
            return WAIT;
        }
        if (!wrk->isok())
            WPRC_ERR("Rotate fail");
        
    WPRC_RUN
        
        if (m_by == 0)
            return END;
        
        if (!tr) {
            // пишем заголовок - время старта и номер прыга
            FileTrack::head_t th;
            th.id = esp_random();
            th.jmpnum = jmp.count();
            if (jmp.state() == LOGJMP_NONE) // в случае, если прыг не начался (включение трека до начала прыга),
                th.jmpnum ++;          // за номер прыга считаем следующий
            th.jmpkey = jmp.key();
            th.tmbeg = tmNow(jmpPreInterval(prelogcur));

            if (!tr.create(th))
                WPRC_ERR("Track file create fail");
            
            return RUN;
        }
        
        //CONSOLE("diff: %d", jmpPreCursor()-prelogcur);
        if (prelogcur == jmpPreCursor())
            return WAIT;
        
        prelogcur++;
        auto ti = jmpPreLog(prelogcur);
        
        tmoffset += ti.tmoffset;
        ti.tmoffset = tmoffset;

        ti.msave = utm() / 1000;

        if (!tr.add(ti))
            WPRC_ERR("track save fail");
        
        //CONSOLE("save[%d]: %d", *prelogcur, tmoffset);
        
        if (!useext && (((m_cnt++) & 0b111) == 0) && !trkCheckAvail(false))
            return END;
        
    WPRC(RUN)
    }
    
    void end() {
        tr.close();

        if (cfg.d().navontrkrec)
            gpsOff(NAV_PWRBY_TRKREC);
        if (useext)
            fileExtStop();
        CONSOLE("track(0x%08x) stopped", this);
    }
};


/* ------------------------------------------------------------------------------------------- *
 *  Запуск трекинга
 * ------------------------------------------------------------------------------------------- */
static Wrk2Proc<trkWrk> _trk;

bool trkStart(uint8_t by, uint16_t old) {
    if (by == 0)
        return false;

    if (_trk)
        _trk->byadd(by);
    else
        _trk = wrk2Run<trkWrk>(by, old);

    return _trk;
}

/* ------------------------------------------------------------------------------------------- *
 *  Остановка
 * ------------------------------------------------------------------------------------------- */
void trkStop(uint8_t by) {
    if (_trk)
        _trk->bydel(by);
}

/* ------------------------------------------------------------------------------------------- *
 *  Текущее состояние
 * ------------------------------------------------------------------------------------------- */
bool trkRunning(uint8_t by) {
    return _trk;
}
