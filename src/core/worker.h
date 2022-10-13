/*
    Worker
*/

#ifndef _core_worker_H
#define _core_worker_H

#include <stdint.h>
//#include <stdlib.h>

/* ------------------------------------------------------------------------------------------- *
 *  Базовый класс для каждого воркера
 * ------------------------------------------------------------------------------------------- */

#define WRK_CLASS(key)           WrkProc_ ## key

#define WRK_DEFINE(key)          class WRK_CLASS(key) : public WrkProc

#define WRK_PROCESS  \
    public: \
        state_t process() { \
            switch (m_line) { \
                case 0: {

#define WRK_RETURN_RUN           return STATE_RUN
#define WRK_RETURN_WAIT          return STATE_WAIT
#define WRK_RETURN_END           return STATE_END

#define WRK_BREAK_RUN            m_line = __LINE__; WRK_RETURN_RUN;  } case __LINE__: {
#define WRK_BREAK_WAIT           m_line = __LINE__; WRK_RETURN_WAIT; } case __LINE__: {

#define WRK_END \
                WRK_RETURN_END; \
            } \
        } \
        \
        CONSOLE("Worker line fail: %d", m_line); \
        return STATE_END; \
    }


class WrkProc {
    public:
        typedef uint8_t key_t;
        
        typedef struct {
            WrkProc *proc;
            bool autodestroy;
            bool needwait;
            bool needend;
        } elem_t;
        
        typedef enum {
            STATE_WAIT,
            STATE_RUN,
            STATE_END
        } state_t;
        
        virtual bool every() { return true; }
        virtual state_t process() = 0;
        virtual void end() {};
    
    protected:
        uint32_t m_line = 0;
};

bool wrkEmpty();

void _wrkAdd(WrkProc::key_t key, WrkProc *proc, bool autodestroy = true);
#define wrkRun(key, ...)     _wrkAdd(WRKKEY_ ## key, new WRK_CLASS(key)(__VA_ARGS__))

WrkProc::key_t _wrkAddRand(WrkProc::key_t key_min, WrkProc::key_t key_max, WrkProc *proc, bool autodestroy = true);
#define wrkRand(key, ...)    wrkAddRand(WRKKEY_RAND_MIN, WRKKEY_RAND_MAX, new WRK_CLASS(key)(__VA_ARGS__));

void _wrkDel(WrkProc::key_t key);
#define wrkStop(key)         _wrkDel(WRKKEY_ ## key)

bool _wrkExists(WrkProc::key_t key);
#define wrkExists(key)       _wrkExists(WRKKEY_ ## key)

WrkProc *_wrkGet(WrkProc::key_t key);
#define wrkGet(key)          reinterpret_cast<WRK_CLASS(key) *>(  _wrkGet(WRKKEY_ ## key) )

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);

#endif // _core_worker_H
