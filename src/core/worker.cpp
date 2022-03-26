/*
    Worker
*/

#include "worker.h"

#include <Arduino.h> // millis()

#include <map>

/////////

void WorkerProc::settimer(uint32_t val) {
    m_timer = val+1;
}

void WorkerProc::clrtimer() {
    m_timer = 0;
}

void WorkerProc::dectimer() {
    if (m_timer > 1)
        m_timer --;
}

/////////

typedef std::map<WorkerProc::key_t, WorkerProc::elem_t> worker_list_t;

static worker_list_t wrkall;

void wrkAdd(WorkerProc::key_t key, WorkerProc *proc, bool autodel) {
    wrkDel(key);
    if (proc != NULL) {
        wrkall[key] = { proc, autodel };
        proc->begin();
    }
}

WorkerProc::key_t wrkAddRand(WorkerProc::key_t key_min, WorkerProc::key_t key_max, WorkerProc *proc, bool autodel) {
    for (auto key = key_min; key <= key_max; key++)
        if (wrkall.find(key) == wrkall.end()) {
            wrkAdd(key, proc, autodel);
            return key;
        }
    
    return 0;
}

static void wrkDel(worker_list_t::iterator it) {
    if (it == wrkall.end())
        return;
    auto &wrk = it->second;
    wrk.proc->end();
    if (wrk.autodel)
        delete wrk.proc;
    wrkall.erase(it);
}

void wrkDel(WorkerProc::key_t key) {
    wrkDel(wrkall.find(key));
}

bool wrkExists(WorkerProc::key_t key) {
    return wrkall.find(key) != wrkall.end();
}

WorkerProc *wrkGet(WorkerProc::key_t key) {
    auto it = wrkall.find(key);
    if (it == wrkall.end())
        return NULL;
    return it->second.proc;
}

bool wrkEmpty() {
    return wrkall.size() == 0;
}

void wrkProcess(uint32_t tmmax) {
    if (wrkall.size() == 0)
        return;
    
    for (auto &it : wrkall) {
        // сбрасываем флаг needwait
        it.second.needwait = false;
        // timer
        it.second.proc->dectimer();
    }
    
    uint32_t beg = millis();
    bool run = true;
    
    while (run) {
        run = false;
        for(auto it = wrkall.begin(), itnxt = it; it != wrkall.end(); it = itnxt) {
            itnxt++; // такие сложности, чтобы проще было удалить текущий элемент
            if (it->second.needwait)
                continue;
            switch (it->second.proc->process()) {
                case WorkerProc::STATE_WAIT:
                    // При возвращении STATE_WAIT мы больше не должны
                    // выполнять этот процесс в текущем вызове wrkProcess()
                    it->second.needwait = true;
                    break;
                
                case WorkerProc::STATE_RUN:
                    run = true;
                    break;
                
                case WorkerProc::STATE_END:
                    wrkDel(it);
                    break;
            }
            
            if ((millis()-beg) >= tmmax)
                return;
        }
    }
}
