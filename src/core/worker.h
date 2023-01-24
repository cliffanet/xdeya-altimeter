/*
    Worker
*/

#ifndef _core_worker_H
#define _core_worker_H

#include <stdint.h>
//#include <stdlib.h>
#include <memory>

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
        virtual void remove() {};
        
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

class Wrk2 {
    public:
        // статусы выхода из выполнения воркера
        typedef enum {
            WAIT,         // в этом цикле больше не надо выполнять этот воркер
            RUN,          // этот процесс надо выполнять и дальше, если ещё есть время на это
            END,          // завершить воркер
        } state_t;

        typedef void * key_t;
        typedef std::shared_ptr<Wrk2> run_t;
    
        virtual state_t run() = 0;
        virtual void end() {};

        typedef enum {
            O_INLIST = 0,       // процесс находится в списке воркеров
            O_NEEDWAIT,         // не выполнять это воркер в текущем цикле обработки wrkProcess() - воркер что-то ждёт
            O_NEEDFINISH,       // воркер надо завершить (выполнить end()) и больше не выполнять
            O_FINISHED          // завершённый воркер - end() уже выполнен и этот воркер больше не надо выполнять совсем
        } opts_t;
        
        uint16_t    opts()              const { return __opts; }
        bool        opt(opts_t fl)      const { return (__opts & (1 << fl)) > 0; }
        void        optclr()            { __opts = 0; };
        void        optclr(opts_t fl)   { __opts = (1 << fl); };
        void        optset(opts_t fl)   { __opts |= (1 << fl); }
        void        optdel(opts_t fl)   { __opts &= ~static_cast<uint16_t>(1 << fl); }
    
    protected:
        uint32_t __line = 0;
    private:
        uint16_t __opts = 0;
};

template <class T>
class Wrk2Proc {
    public:
        Wrk2Proc() { }
        Wrk2Proc(const std::shared_ptr<T> &ws) : _w(ws) { }

        Wrk2::key_t key()   const { return _w.get(); }
        bool        isrun() const { return _w && !_w->opt(Wrk2::O_FINISHED); }
        operator    bool()  const { return isrun(); }

        bool        stop() {
            // Заставляет процесс остановиться,
            // но не отвязывается от него, можно получить результат

            if (!_w || _w->opt(Wrk2::O_FINISHED))
                return false;
            _w->optset(Wrk2::O_NEEDFINISH);
            return false;
        }
        bool        reset() {
            // Отвязывается от процесса, получить результат
            // будет уже нельзя, но не останавливает его
            if (!_w)
                return false;
            _w.reset();
            return true;
        }
        bool        term() {
            // принудительное завершение и отвязка от процесса
            // в одно действие
            if (!_w)
                return false;
            if (!_w->opt(Wrk2::O_FINISHED))
                _w->optset(Wrk2::O_NEEDFINISH);
            _w.reset();
            return true;
        }

        T& operator*() const {
            return *_w;
        }
        T* operator->() const {
            return _w.get();
        }

    private:
        std::shared_ptr<T> _w;
};

bool _wrk2Add(Wrk2::run_t ws);

template<typename T, typename... _Args>
inline Wrk2Proc<T> wrk2Run(_Args&&... __args) {
    std::shared_ptr<T> ws = std::make_shared<T>(__args...);
    _wrk2Add(ws);
    return Wrk2Proc<T>(ws);
}

#define WPROC \
            switch (__line) { \
                case 0: {

#define WPRC_BREAK              __line = __LINE__; }                case __LINE__: {
#define WPRC_RUN                __line = __LINE__; return RUN; }    case __LINE__: {
#define WPRC_WAIT               __line = __LINE__; return WAIT; }   case __LINE__: {
#define WPRC(ret) \
                    return ret; \
                } \
            } \
            \
            CONSOLE("Worker line fail: %d", __line); \
            return END;


// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);

bool wrkEmpty();

#endif // _core_worker_H
