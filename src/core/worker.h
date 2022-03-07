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

        virtual void begin() {};
        virtual state_t process() = 0;
        virtual void end() {};
};

void wrkAdd(WorkerProc::key_t key, WorkerProc *proc, bool autodel = true);
WorkerProc::key_t wrkAddRand(WorkerProc::key_t key_min, WorkerProc::key_t key_max, WorkerProc *proc, bool autodel = true);
void wrkDel(WorkerProc::key_t key);
bool wrkExists(WorkerProc::key_t key);
bool wrkEmpty();

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);

#endif // _core_worker_H
