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
class WorkerProc {
    public:
    
        typedef uint8_t key_t;
        
        typedef struct {
            WorkerProc *proc;
            bool autodel;
            bool needwait;
        } elem_t;
        
        typedef enum {
            STATE_WAIT,
            STATE_RUN,
            STATE_END
        } state_t;
        
        virtual ~WorkerProc() {}    // Для корректного срабатывания деструкторов-потомков,
                                    // этот деструктор должен быть виртуальным

        virtual void begin() {};
        virtual state_t process() = 0;
        virtual void end() {};
        
        void settimer(uint32_t val);
        void clrtimer();
        void dectimer();
        bool istimeout() const { return m_timer == 1; }
        uint32_t timer() const { return m_timer > 0 ? m_timer-1 : 0; }
        
        uint32_t op() const { return m_op; }
        void setop(uint32_t op);
        void next();
        void next(uint32_t timer);
    
    private:
        uint32_t m_timer = 0;
        uint32_t m_op = 0;
};

void wrkAdd(WorkerProc::key_t key, WorkerProc *proc, bool autodel = true);
WorkerProc::key_t wrkAddRand(WorkerProc::key_t key_min, WorkerProc::key_t key_max, WorkerProc *proc, bool autodel = true);
void wrkDel(WorkerProc::key_t key);
bool wrkExists(WorkerProc::key_t key);
WorkerProc *wrkGet(WorkerProc::key_t key);
bool wrkEmpty();

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);



/* ------------------------------------------------------------------------------------------- *
 *  Базовый класс для каждого воркера
 * ------------------------------------------------------------------------------------------- */

#define WORKER_DEFINE(name)         class CWorker_ ## name : public CWorker

#define WORKER_PROCESS  \
    public: \
        state_t process() { \
            switch (m_line) { \
                case 0: {

#define WORKER_BREAK_RUN            m_line = __LINE__; return STATE_RUN;  } case __LINE__: {
#define WORKER_BREAK_WAIT           m_line = __LINE__; return STATE_WAIT; } case __LINE__: {

#define WORKER_END \
                return STATE_END; \
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

void _wrkAdd(CWorker::key_t key, CWorker *proc, bool autodestroy = true);
#define workerAdd(key, class, ...)      _wrkAdd(WORKER_ ## key, new CWorker_ ## class(), ##__VA_ARGS__)

CWorker::key_t _wrkAddRand(CWorker::key_t key_min, CWorker::key_t key_max, CWorker *proc, bool autodestroy = true);
void _wrkDel(CWorker::key_t key);
bool _wrkExists(CWorker::key_t key);
CWorker *_wrkGet(CWorker::key_t key);

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess2(uint32_t tmmax = 50);

#endif // _core_worker_H
