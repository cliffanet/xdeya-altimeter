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
class Wrk {
    public:
        // статусы выхода из выполнения воркера
        typedef enum {
            DLY,          // в этом цикле больше не надо выполнять этот воркер
            RUN,          // этот процесс надо выполнять и дальше, если ещё есть время на это
            END,          // завершить воркер
        } state_t;

        typedef void * key_t;
        typedef std::shared_ptr<Wrk> run_t;

        virtual void timer() {}
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

// воркер, возвращающий значение
template <class T>
class WrkRet : public Wrk {
    public:
        const T &ret() const { return ret; }
    private:
        T m_ret;
};

// воркер, возвращающий bool: isok()
class WrkOk : public Wrk {
    public:
        bool isok() const { return m_isok; }
    protected:
        state_t ok() {
            m_isok=true;
            return END;
        }
        state_t err() {
            m_isok=false;
            return END;
        }
        bool m_isok = false;
};

// воркер процессинг, за которым можно
// наблюдать процент выполненного, а так же,
// получать статус isok()
class WrkCmpl : public WrkOk {
    public:
        typedef struct {
            uint32_t val, sz;
        } cmpl_t;

        WrkCmpl() :
            m_cmpl({ 0, 0 })
            {}

        const cmpl_t& cmpl() const { return m_cmpl; };

    protected:
        cmpl_t m_cmpl;
};

// Шаблон класса-контроллера за воркером
template <class T>
class WrkProc {
    public:
        WrkProc() { }
        WrkProc(const std::shared_ptr<T> &ws) : _w(ws) { }

        Wrk::key_t key()   const { return _w.get(); }
        bool        isrun() const { return _w && !_w->opt(Wrk::O_FINISHED); }
        bool        valid() const { return _w ? true : false; }
        operator    bool()  const { return isrun(); }

        bool        stop() {
            // Заставляет процесс остановиться,
            // но не отвязывается от него, можно получить результат

            if (!_w || _w->opt(Wrk::O_FINISHED))
                return false;
            _w->optset(Wrk::O_NEEDFINISH);
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
            if (!_w->opt(Wrk::O_FINISHED))
                _w->optset(Wrk::O_NEEDFINISH);
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

bool _wrkAdd(Wrk::run_t ws);

// Кастомный wrkRun, когда нам надо запустить воркер
// определённого класса, но вернуть контроллер WrkProc<T>,
// который обернёт класс родителя этого воркера.
// Это может быть полезным, чтобы
// обобщить в один контроллер разные процессы одного базового класса
template<typename Twrk, typename Tret, typename... _Args>
inline WrkProc<Tret> wrkRun(_Args&&... __args) {
    std::shared_ptr<Twrk> ws = std::make_shared<Twrk>(__args...);
    _wrkAdd(ws);
    return WrkProc<Tret>(ws);
}

// Классическая обертка воркера в контроллер WrkProc<T>
template<typename T, typename... _Args>
inline WrkProc<T> wrkRun(_Args&&... __args) {
    return wrkRun<T, T>(__args...);
}

#define WPROC \
            switch (__line) { \
                case 0: {

#define WPRC_BREAK              __line = __LINE__; }                case __LINE__: {
#define WPRC_RUN                __line = __LINE__; return RUN; }    case __LINE__: {
#define WPRC_DLY                __line = __LINE__; return DLY; }    case __LINE__: {
#define WPRC(ret) \
                    return ret; \
                } \
            } \
            \
            CONSOLE("Worker line fail: %d", __line); \
            return END;

#define WPRC_AWAIT(v, T, ...) \
                v = wrkRun<T>(__VA_ARGS__); \
                WPRC_BREAK \
                if (v.isrun()) return DLY;




// исполнение всех существующих воркеров в течение времени tmmax (мс)
void wrkProcess(uint32_t tmmax = 50);

bool wrkEmpty();

#endif // _core_worker_H
