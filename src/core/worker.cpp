/*
    Worker
*/

#include "worker.h"

#include <Arduino.h> // millis()

#include <map>


/////////

typedef std::map<WrkProc::key_t, WrkProc::elem_t> worker_list_t;

static worker_list_t wrkall;

bool wrkEmpty() {
    return wrkall.size() == 0;
}

void _wrkAdd(WrkProc::key_t key, WrkProc *proc, bool autodestroy) {
    _wrkDel(key);
    if (proc != NULL)
        wrkall[key] = { proc, autodestroy };
}

WrkProc::key_t _wrkAddRand(WrkProc::key_t key_min, WrkProc::key_t key_max, WrkProc *proc, bool autodestroy) {
    for (auto key = key_min; key <= key_max; key++)
        if (wrkall.find(key) == wrkall.end()) {
            _wrkAdd(key, proc, autodestroy);
            return key;
        }
    
    return 0;
}

static void _wrkDel(worker_list_t::iterator it) {
    if (it == wrkall.end())
        return;
    auto &wrk = it->second;
    wrk.proc->end();
    if (wrk.autodestroy)
        delete wrk.proc;
    wrkall.erase(it);
}

void _wrkDel(WrkProc::key_t key) {
    //_wrkDel(wrkall.find(key));
    auto it = wrkall.find(key);
    if (it != wrkall.end())
        it->second.needend = true;
}

bool _wrkExists(WrkProc::key_t key) {
    return wrkall.find(key) != wrkall.end();
}

WrkProc *_wrkGet(WrkProc::key_t key) {
    auto it = wrkall.find(key);
    if (it == wrkall.end())
        return NULL;
    return it->second.proc;
}

void wrkProcess(uint32_t tmmax) {
    if (wrkall.size() == 0)
        return;
    
    for (auto &it : wrkall)
        // сбрасываем флаг needwait
        it.second.needwait = false;
    
    uint32_t beg = millis();
    bool run = true;
    
    while (run) {
        run = false;
        for(auto it = wrkall.begin(), itnxt = it; it != wrkall.end(); it = itnxt) {
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
                    case WrkProc::STATE_WAIT:
                        // При возвращении STATE_WAIT мы больше не должны
                        // выполнять этот процесс в текущем вызове wrkProcess()
                        it->second.needwait = true;
                        break;
                
                    case WrkProc::STATE_RUN:
                        run = true;
                        break;
                
                    case WrkProc::STATE_END:
                        it->second.needend = true;
                        run = true;
                        break;
                }
            
            if ((millis()-beg) >= tmmax)
                return;
        }
    }
}
