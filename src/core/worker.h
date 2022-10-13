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

#define WORKER_CLASS(key)           CWorker_ ## key

#define WORKER_DEFINE(key)          class WORKER_CLASS(key) : public CWorker

#define WORKER_PROCESS  \
    public: \
        state_t process() { \
            switch (m_line) { \
                case 0: {

#define WORKER_RETURN_RUN           return STATE_RUN;
#define WORKER_RETURN_WAIT          return STATE_WAIT;
#define WORKER_RETURN_END           return STATE_END;

#define WORKER_BREAK_RUN            m_line = __LINE__; WORKER_RETURN_RUN;  } case __LINE__: {
#define WORKER_BREAK_WAIT           m_line = __LINE__; WORKER_RETURN_WAIT; } case __LINE__: {

#define WORKER_END \
                WORKER_RETURN_END; \
            } \
        } \
        \
        CONSOLE("Worker line fail: %d", m_line); \
        return STATE_END; \
    }


class CWorker {
    public:
        typedef uint8_t key_t;
        
        typedef struct {
            CWorker *proc;
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

void _wrkAdd(CWorker::key_t key, CWorker *proc, bool autodestroy = true);
#define workerRun(key, ...)     _wrkAdd(WORKER_ ## key, new WORKER_CLASS(key)(__VA_ARGS__))

CWorker::key_t _wrkAddRand(CWorker::key_t key_min, CWorker::key_t key_max, CWorker *proc, bool autodestroy = true);
#define workerRand(key, ...)    wrkAddRand(WORKER_RAND_MIN, WORKER_RAND_MAX, new WORKER_CLASS(key)(__VA_ARGS__));

void _wrkDel(CWorker::key_t key);
#define workerStop(key)         _wrkDel(WORKER_ ## key)

bool _wrkExists(CWorker::key_t key);
#define workerExists(key)       _wrkExists(WORKER_ ## key)

CWorker *_wrkGet(CWorker::key_t key);
#define workerGet(key)          reinterpret_cast<WORKER_CLASS(key) *>(  _wrkGet(WORKER_ ## key) )

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess2(uint32_t tmmax = 50);

#endif // _core_worker_H
