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

#define WRK_CLASS(key)          WrkProc_ ## key

#define WRK_DEFINE(key)         class WRK_CLASS(key) : public WrkProc

#define WRK_PROCESS  \
    public: \
        state_t process() { \
            switch (m_line) { \
                case 0: {

#define WRK_RETURN_RUN          return STATE_RUN
#define WRK_RETURN_WAIT         return STATE_WAIT
#define WRK_RETURN_FINISH       return STATE_FINISH
#define WRK_RETURN_END          return STATE_END

#define WRK_BREAK               m_line = __LINE__; }                    case __LINE__: {
#define WRK_BREAK_RUN           m_line = __LINE__; WRK_RETURN_RUN; }    case __LINE__: {
#define WRK_BREAK_FINISH        m_line = __LINE__; WRK_RETURN_FINISH; } case __LINE__: {
#define WRK_BREAK_WAIT          m_line = __LINE__; WRK_RETURN_WAIT; }   case __LINE__: {

#define WRK_END \
                WRK_RETURN_END; \
            } \
        } \
        \
        CONSOLE("Worker line fail: %d", m_line); \
        WRK_RETURN_END; \
    }

#define WRK_FINISH \
                WRK_RETURN_FINISH; \
            } \
        } \
        \
        CONSOLE("Worker line fail: %d", m_line); \
        WRK_RETURN_END; \
    }


class WrkProc {
    public:
        typedef uint8_t key_t;
        
        // статусы выхода из выполнения воркера (у методов every и process)
        typedef enum {
            STATE_WAIT,         // в этом цикле больше не надо выполнять этот воркер
            STATE_RUN,          // этот процесс надо выполнять и дальше, если ещё есть время на это
            STATE_FINISH,       // завершить воркер (в т.ч. выполнить end), но не удалять из списка воркеров
                                // полезно для воркеров, чьё состояние ещё требуется изучать даже после завершения их функции
            STATE_END           // так же, как и STATE_FINISH, но удаляет воркер из списка (окончательное завершение),
                                // кроме ситуации, если установлен флаг O_NOREMOVE (этот флаг перекрывает действие STATE_END)
        } state_t;
        
        virtual state_t every() { return STATE_RUN; }
        virtual state_t process() = 0;
        virtual void end() {};
        
        typedef enum {
            O_NODESTROY,        // не уничтожать объект при удалении воркера из списка
            O_NOREMOVE,         // не удалять объект из списка при завершении даже если process() вернул STATE_END
                                // этот флаг удобно установить снаружи воркера, чтобы не дать ему удалиться,
                                // даже если воркер захочет это сделать
            O_NEEDWAIT,         // не выполнять это воркер в текущем цикле обработки wrkProcess() - воркер что-то ждёт
            O_NEEDFINISH,       // воркер надо завершить (выполнить end()) и больше не выполнять, но не удалять из списка
            O_NEEDREMOVE,       // воркер надо завершить и удалить из списка
            O_FINISHED          // завершённый воркер - end() уже выполнен и этот воркер больше не надо выполнять совсем
        } opts_t;
        
        uint32_t    opts()              const { return m_opts; }
        bool        opt(opts_t fl)      const { return (m_opts & (1 << fl)) > 0; }
        bool        optset(opts_t fl)   { return m_opts |= (1 << fl); }
        bool        optdel(opts_t fl)   { return m_opts &= ~static_cast<uint16_t>(1 << fl); }
        
        bool        isrun()             const { return !opt(O_FINISHED); }
    
    protected:
        uint32_t m_line = 0;
        uint16_t m_opts = 0;
};

bool wrkEmpty();

void _wrkAdd(WrkProc::key_t key, WrkProc *wrk);
#define wrkRun(key, ...)    _wrkAdd(WRKKEY_ ## key, new WRK_CLASS(key)(__VA_ARGS__))

WrkProc::key_t _wrkAddRand(WrkProc::key_t key_min, WrkProc::key_t key_max, WrkProc *wrk);
#define wrkRand(key, ...)   _wrkAddRand(WRKKEY_RAND_MIN, WRKKEY_RAND_MAX, new WRK_CLASS(key)(__VA_ARGS__));

bool _wrkDel(WrkProc::key_t key);
#define wrkStop(key)        _wrkDel(WRKKEY_ ## key)

bool _wrkExists(WrkProc::key_t key);
#define wrkExists(key)      _wrkExists(WRKKEY_ ## key)

WrkProc *_wrkGet(WrkProc::key_t key);
#define wrkGetBase(key)     _wrkGet(WRKKEY_ ## key)
#define wrkGet(key)         reinterpret_cast<WRK_CLASS(key) *>(  _wrkGet(WRKKEY_ ## key) )

// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);

#endif // _core_worker_H
