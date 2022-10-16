/*
    Worker
*/

#include "worker.h"

#include <Arduino.h> // millis()

#include <map>

#include "../log.h" // временно для отладки


/////////

typedef std::map<WrkProc::key_t, WrkProc *> worker_list_t;

static worker_list_t wrkall;

bool wrkEmpty() {
    return wrkall.size() == 0;
}

void _wrkAdd(WrkProc::key_t key, WrkProc *wrk) {
    _wrkDel(key);
    if (wrk != NULL)
        wrkall[key] = wrk;
    CONSOLE("key: %d, ptr: %x", key, wrk);
}

WrkProc::key_t _wrkAddRand(WrkProc::key_t key_min, WrkProc::key_t key_max, WrkProc *wrk) {
    for (auto key = key_min; key <= key_max; key++)
        if (wrkall.find(key) == wrkall.end()) {
            _wrkAdd(key, wrk);
            return key;
        }
    
    return 0;
}


static inline void _wrkFinish(worker_list_t::iterator it) {
    auto &wrk = it->second;
    if (wrk->opt(WrkProc::O_FINISHED))
        return;

    CONSOLE("key: %d, ptr: %x", it->first, wrk);
    wrk->end();
    wrk->optset(WrkProc::O_FINISHED);
    wrk->optdel(WrkProc::O_NEEDFINISH);
    CONSOLE("opts: %02x", wrk->opts());
}

static void _wrkRemove(worker_list_t::iterator it) {
    _wrkFinish(it);
        
    auto wrk = it->second;
    CONSOLE("key: %d, ptr: %x, opts: %02x", it->first, wrk, wrk->opts());
    wrkall.erase(it);
    wrk->remove();
    if (!wrk->opt(WrkProc::O_NODESTROY))
        delete wrk;
}

bool _wrkDel(WrkProc::key_t key) {
    auto it = wrkall.find(key);
    if (it == wrkall.end())
        return false;
        
    auto &wrk = it->second;
    //if (wrk->opt(WrkProc::O_FINISHED))
    //    _wrkRemove(it);
    //else
    // оставим удаление из списка основному процессу wrkProcess(),
    // иначе, создаются ненужные петли, когда мы из одного воркера удаляем следующий в списке,
    // и рабочий цикл в wrkProcess() уже работает некорректно и пытается повторно убить уже
    // убитый процесс.
        wrk->optset(WrkProc::O_NEEDREMOVE);
    
    return true;
}

bool _wrkExists(WrkProc::key_t key) {
    return wrkall.find(key) != wrkall.end();
}

WrkProc *_wrkGet(WrkProc::key_t key) {
    auto it = wrkall.find(key);
    if (it == wrkall.end())
        return NULL;
    return it->second;
}

void wrkProcess(uint32_t tmmax) {
    if (wrkall.size() == 0)
        return;
    
    for (auto &it : wrkall)
        // сбрасываем флаг needwait
        it.second->optdel(WrkProc::O_NEEDWAIT);
    
    uint32_t beg = millis();
    bool run = true;
    
    while (run) {
        run = false;
        for(auto it = wrkall.begin(), itnxt = it; it != wrkall.end(); it = itnxt) {
            itnxt++; // такие сложности, чтобы проще было удалить текущий элемент
            auto &wrk = it->second;
            
            if ( wrk->opt(WrkProc::O_NEEDREMOVE) ) {
                _wrkRemove(it);
                continue;
            } else
            if ( wrk->opt(WrkProc::O_FINISHED) )
                continue;
            
            if ( wrk->opt(WrkProc::O_NEEDFINISH) )
                _wrkFinish(it);
            else
            if ( ! wrk->opt(WrkProc::O_NEEDWAIT) )
                switch ( wrk->every() ) {
                    case WrkProc::STATE_WAIT:
                        wrk->optset(WrkProc::O_NEEDWAIT);
                        break;
                        
                    case WrkProc::STATE_RUN:
                        switch ( wrk->process() ) {
                            case WrkProc::STATE_WAIT:
                                wrk->optset(WrkProc::O_NEEDWAIT);
                                break;
                
                            case WrkProc::STATE_RUN:
                                run = true;
                                break;
                
                            case WrkProc::STATE_FINISH:
                                wrk->optset(WrkProc::O_NEEDFINISH);
                                run = true;
                                break;
                
                            case WrkProc::STATE_END:
                                wrk->optset( wrk->opt(WrkProc::O_NOREMOVE) ? WrkProc::O_NEEDFINISH : WrkProc::O_NEEDREMOVE );
                                run = true;
                                break;
                        }
                        break;
                
                    case WrkProc::STATE_FINISH:
                        wrk->optset(WrkProc::O_NEEDFINISH);
                        run = true;
                        break;
                        
                    case WrkProc::STATE_END:
                        wrk->optset( wrk->opt(WrkProc::O_NOREMOVE) ? WrkProc::O_NEEDFINISH : WrkProc::O_NEEDREMOVE );
                        run = true;
                        break;
                };
                
            
            if ((millis()-beg) >= tmmax)
                return;
        }
    }
}
