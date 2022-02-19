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
void wrkDel(WorkerProc::key_t key);
bool wrkEmpty();
void wrkProcess(uint32_t tmmax = 50);

#endif // _core_worker_H
