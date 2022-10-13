/*
    Worker
*/

#include "worker.h"

#include <Arduino.h> // millis()

#include <map>


/////////

typedef std::map<CWorker::key_t, CWorker::elem_t> cworker_list_t;

static cworker_list_t wall;

bool wrkEmpty() {
    return wall.size() == 0;
}

void _wrkAdd(CWorker::key_t key, CWorker *proc, bool autodestroy) {
    _wrkDel(key);
    if (proc != NULL)
        wall[key] = { proc, autodestroy };
}

CWorker::key_t _wrkAddRand(CWorker::key_t key_min, CWorker::key_t key_max, CWorker *proc, bool autodestroy) {
    for (auto key = key_min; key <= key_max; key++)
        if (wall.find(key) == wall.end()) {
            _wrkAdd(key, proc, autodestroy);
            return key;
        }
    
    return 0;
}

static void _wrkDel(cworker_list_t::iterator it) {
    if (it == wall.end())
        return;
    auto &wrk = it->second;
    wrk.proc->end();
    if (wrk.autodestroy)
        delete wrk.proc;
    wall.erase(it);
}

void _wrkDel(CWorker::key_t key) {
    //_wrkDel(wall.find(key));
    auto it = wall.find(key);
    if (it != wall.end())
        it->second.needend = true;
}

bool _wrkExists(CWorker::key_t key) {
    return wall.find(key) != wall.end();
}

CWorker *_wrkGet(CWorker::key_t key) {
    auto it = wall.find(key);
    if (it == wall.end())
        return NULL;
    return it->second.proc;
}

void wrkProcess2(uint32_t tmmax) {
    if (wall.size() == 0)
        return;
    
    for (auto &it : wall)
        // сбрасываем флаг needwait
        it.second.needwait = false;
    
    uint32_t beg = millis();
    bool run = true;
    
    while (run) {
        run = false;
        for(auto it = wall.begin(), itnxt = it; it != wall.end(); it = itnxt) {
            itnxt++; // такие сложности, чтобы проще было удалить текущий элемент
            
            if ( it->second.needend )
                _wrkDel(it);
            else
            if ( it->second.needwait ) {
                
            }
            else
            if ( ! it->second.proc->every() ) {
                // при ошибке выполнения every() - делаем обычное завершение
                it->second.needend = true;
                run = true;
            }
            else
                switch (it->second.proc->process()) {
                    case CWorker::STATE_WAIT:
                        // При возвращении STATE_WAIT мы больше не должны
                        // выполнять этот процесс в текущем вызове wrkProcess()
                        it->second.needwait = true;
                        break;
                
                    case CWorker::STATE_RUN:
                        run = true;
                        break;
                
                    case CWorker::STATE_END:
                        it->second.needend = true;
                        run = true;
                        break;
                }
            
            if ((millis()-beg) >= tmmax)
                return;
        }
    }
}
