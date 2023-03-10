

#include "sys.h"

#if HWVER >= 5
#include "../log.h"
#include "../core/file.h"
#include "../view/text.h"
#include "../clock.h"

#include <SD.h>

#define ERR(s)              err(PSTR(TXT_BKPSD_ERR_ ## s))

static bool dirCreate(char *name) {
    auto tm = tmNow();
    snprintf_P(
        name, WRKBKP_DIRLEN,
        PSTR("/xdeya_%04d-%02d-%02d_%02d%02d%02d"),
        tm.year, tm.mon, tm.day,
        tm.h, tm.m, tm.s
    );

    return SD.mkdir(name);
}

Wrk::state_t WrkBkpSDall::run() {
WPROC
    m_dh = dirIntOpen();
    if (!m_dh || !m_dh.isDirectory())
        return ERR(SDINIT);
    
WPRC_RUN
    File file = m_dh.openNextFile();
    if (file) {
        if (!file.isDirectory()) {
            CONSOLE("file[%s]\t= %d bytes", file.name(), file.size());
            m_cmpl.sz += file.size();
        }
        return RUN;
    }
    CONSOLE("size: %d", m_cmpl.sz);

WPRC_RUN
    if (!fileExtInit())
        return ERR(SDINIT);

WPRC_RUN
    if (!dirCreate(m_dirname))
        return ERR(MKDIR);

WPRC_RUN
    m_dh = dirIntOpen();

WPRC_RUN
    if (!m_fhi) {
        m_fhi = m_dh.openNextFile();
        if (!m_fhi) {
            m_cmpl.sz = 0;
            return ok(PSTR(TXT_BKPSD_FINISH));
        }
        
        if (m_fhi.isDirectory()) {
            m_fhi.close();
            return RUN;
        }

        char fname[128];
        snprintf_P(fname, sizeof(fname), PSTR("%s/%s"), m_dirname, m_fhi.name());
        m_fho = SD.open(fname, FILE_WRITE);
        if (!m_fho) {
            CONSOLE("file(%s) create fail", fname);
            return ERR(FILECREATE);
        }
        CONSOLE("file beg: %s (%d bytes)", m_fhi.name(), m_fhi.size());
        
        return RUN;
    }

    if (m_fhi.available() <= 0) {
        CONSOLE("file end");
        m_fhi.close();
        m_fho.close();
        return RUN;
    }

    uint8_t buf[256];
    size_t sz = m_fhi.read(buf, sizeof(buf));
    if (sz <= 0) {
        CONSOLE("file(%s) read err on pos: %d", m_fhi.name(), m_fhi.position());
        return ERR(FILEREAD);
    }

    size_t sz1 = m_fho.write(buf, sz);
    if (sz1 != sz) {
        CONSOLE("file(%s) write err on pos: %d", m_fho.name(), m_fho.position());
        return ERR(FILEWRITE);
    }

    m_cmpl.val += sz;

WPRC(RUN)
};

void WrkBkpSDall::end() {
    fileExtStop();
}

Wrk::state_t WrkBkpSDlast::run() {
WPROC
    if (!fileExtInit())
        return ERR(SDINIT);

WPRC_RUN

    m_isok=true;

WPRC(END)
};

void WrkBkpSDlast::end() {
    fileExtStop();
}

#endif // HWVER >= 5
