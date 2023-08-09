/*
    Alt calculate
*/

#ifndef __altcalc_H
#define __altcalc_H

#include <stdint.h> 

#define AC_DATA_COUNT           40

// ************************************************
//  Параметры для определения direct и state
//
// Порог скорости для режима FLAT (abs, m/s)
// Он же является порогом удержания для режима canopy
#define AC_SPEED_FLAT           1.5
// Порог срабатывания режима canopy,
// т.е. если скорость снижения выше этой, то canopy
// ну или freefall
#define AC_SPEED_CANOPY_I       4
// Порог срабатывания режима freefall безусловный (abs, m/s)
// т.е. если скорость снижения выше этой, то полюбому - ff
#define AC_SPEED_FREEFALL_I     30
// Порог удержания режима freefall,
// т.е. если текущий режим freefall, 
// то он будет удерживаться таким, пока скорость снижения выше этой
#define AC_SPEED_FREEFALL_O     20

// states
typedef enum {
    ACST_INIT = 0,
    ACST_GROUND,
    ACST_TAKEOFF40,
    ACST_TAKEOFF,
    ACST_FREEFALL,
    ACST_CANOPY,
    ACST_LANDING
} ac_state_t;

// direct
typedef enum {
    ACDIR_INIT = 0,
    ACDIR_ERR,
    ACDIR_UP,
    ACDIR_FLAT,
    ACDIR_DOWN
} ac_direct_t;

// ************************************************
//  Параметры для определения direct и state
//
// Время (кол-во тиков), которое должен удерживаться state() > ACST_GROUND для определения, что это подъём
#define AC_JMP_TOFF_COUNT       10
// Время (мс), оторое должен удерживаться state() > ACST_GROUND для определения, что это подъём
#define AC_JMP_TOFF_TIME        7000
// Порог скорости, при которой считаем, что начался прыжок
#define AC_JMP_SPEED_MIN        15
// Время (кол-во тиков), которое должна удерживаться скорость AC_JMP_SPEED_MIN
#define AC_JMP_SPEED_COUNT      5
// Время (мс), которое должна удерживаться скорость AC_JMP_SPEED_MIN
#define AC_JMP_SPEED_TIME       3500
// Порог скорости, при котором произойдёт отмена прыжка, если не закончилось время JMP_SPEED_COUNT
#define AC_JMP_SPEED_CANCEL     10
// Время (кол-во тиков), которое должен удержать state() == ACST_FREEFALL, чтобы считать, что это ACJMP_FREEFALL
#define AC_JMP_FF_COUNT         5
// Время (мс), которое должен удержать state() == ACST_FREEFALL, чтобы считать, что это ACJMP_FREEFALL
#define AC_JMP_FF_TIME          5000
// Время (кол-во тиков), которое мы ждём, чтобы окончательно убедиться, что это не FF
#define AC_JMP_NOFF_COUNT       8
// Время (мс), которое мы ждём, чтобы окончательно убедиться, что это не FF
#define AC_JMP_NOFF_TIME        8000
// Скорость, при котором будет переход из FF в CNP
#define AC_JMP_SPEED_CANOPY     12
// Время (кол-во тиков), которое должна удерживаться скорость AC_JMP_SPEED_CANOPY для перехода из FF в CNP
#define AC_JMP_CNP_COUNT        6
// Время (мс), которое должна удерживаться скорость AC_JMP_SPEED_CANOPY для перехода из FF в CNP
#define AC_JMP_CNP_TIME         6000
// Время (кол-во тиков), которое должен удерживаться state() == ACST_GROUND для перехода в NONE
#define AC_JMP_GND_COUNT        6
// Время (мс), которое должен удерживаться state() == ACST_GROUND для перехода в NONE
#define AC_JMP_GND_TIME         6000

// порог переключения в sqbig (сильные турбуленции)
#define AC_JMP_SQBIG_THRESH     12
#define AC_JMP_SQBIG_MIN        4
// время (кол-во тиков) между элементами профайла начала прыжка
#define AC_JMP_PROFILE_COUNT    10

// режим прыжка
typedef enum {
    ACJMP_INIT = -1,
    ACJMP_NONE,
    ACJMP_TAKEOFF,
    ACJMP_FREEFALL,
    ACJMP_CANOPY
} ac_jmpmode_t;

float press2alt(float pressgnd, float pressure);

typedef struct {
    uint16_t interval;
    float press;
    float alt;
} ac_data_t;

class AltCalc
{
    public:
        // Текущее давление у земли
        const float         pressgnd()  const { return _pressgnd; }
        // Текущее давление
        const float         press()     const { return _c < AC_DATA_COUNT ? _data[_c].press : 0; }
        // суммарный интервал времени, за которое собраны данные
        const uint32_t      interval()  const { return _interval; }
        // Текущая высота
        const float         alt()       const { return _c < AC_DATA_COUNT ? _data[_c].alt : 0; }
        // Среднее значение высоты за AC_DATA_COUNT циклов
        const float         altavg()    const { return _altavg; }
        // Высота по аппроксимации
        const float         altapp()    const { return _ka * _interval + _kb; }
        // Средняя скорость за AC_LEVEL1_COUNT циклов
        const float         speedavg()  const { return _speedavg; }
        // Скорость по аппроксимации м/с
        const float         speedapp()  const { return _ka * 1000; }
        
        // Текущий режим высоты (определяется автоматически)
        const ac_state_t    state()     const { return _state; }
        // Время с предыдущего изменения режима высоты
        const uint32_t      statetm()   const { return _statetm; }
        const uint32_t      statecnt()  const { return _statecnt; }
        void statereset() { _statecnt = 0; _statetm = 0; }
        
        // Направление вертикального движения (вверх/вниз)
        const ac_direct_t   direct()    const { return _dir; }
        // Как долго сохраняется текущее направления движения (в ms)
        const uint32_t      dirtm()     const { return _dirtm; }
        const uint32_t      dircnt()    const { return _dircnt; }
        void dirreset() { _dircnt = 0; _dirtm = 0; }
        
        // Текущий режим прыжка (определяется автоматически и с задержкой)
        const ac_jmpmode_t  jmpmode()   const { return _jmpmode; }
        // Время с предыдущего изменения режима прыжка
        const uint32_t      jmptm()     const { return _jmptm; }
        const uint32_t      jmpcnt()    const { return _jmpcnt; }
        void jmpreset() { _jmpcnt = 0; _jmptm = 0; }
        
        // среднеквадратическое отклонение прямой скорости
        const double        sqdiff()    const { return _sqdiff; }
        // большая турбуленция (высокое среднеквадратическое отклонение)
        const bool          sqbig()     const { return _sqbig; }
        const uint32_t      sqbigtm()   const { return _sqbigtm; }
        const uint32_t      sqbigcnt()  const { return _sqbigcnt; }
        // коэфициент kb - фактически высота в km самом начале бувера вычислений
        const double        kb()  const { return _kb; }
        
        // доступ к элементу data по индексу относительно _с
        // т.е. при i=0 - текущий элемент, при i=-1 или i=AC_DATA_COUNT-1 - самый старый элемент
        const ac_data_t &   data(int8_t i) { return _data[i2i(i)]; };
        
        // очередное полученное значение давления и интервал в ms после предыдущего вычисления
        void tick(float press, uint16_t interval);
        // пересчитать из данных коэфициенты линейной аппроксимации и средние значения
        void dcalc();
        // обновляет текущее состояние, вычисленное по коэфициентам
        // сбрасывает "ноль" высоты в текущие показания и обнуляет все состояния
        void gndreset();
        void gndset(float press, uint16_t interval = 100);

        // временно для отладки выводим данные по отработке профиля начала прыжка
        const uint8_t   ffprof()    const { return _ffprof; }
        const uint32_t  ffproftm()  const { return _ffproftm; }
        const int32_t   ffalt()  const { return _altprof; }
  
    private:
        float _pressgnd = 101325;
        ac_data_t _data[AC_DATA_COUNT];
        uint8_t _c = 0xff;
        double _ka = 0, _kb = 0, _sqdiff = 0;
        float _alt0 = 0, _press0 = 0, _altavg = 0, _altprof = 0, _speedavg = 0;
        uint32_t _interval = 0;
        ac_state_t _state = ACST_INIT;
        ac_direct_t _dir = ACDIR_INIT;
        ac_jmpmode_t _jmpmode = ACJMP_INIT;
        bool _sqbig = false;
        uint8_t _ffprof = 0;
        uint32_t _statecnt = 0, _statetm = 0;
        uint32_t _dircnt = 0, _dirtm = 0;
        uint32_t _jmpcnt = 0, _jmptm = 0;
        uint32_t _jmpccnt = 0, _jmpctm = 0;
        uint32_t _sqbigcnt = 0, _sqbigtm = 0;
        uint32_t _ffprofcnt = 0, _ffproftm = 0;

        ac_state_t stateupdate(uint16_t tinterval);
        // вынесенное отдельно из stateupdate(); пересчёт в режиме takeoff
        void toffupdate(uint16_t tinterval);
        
        int8_t          i2i(int8_t i);
        const int8_t    ifrst() const;
        bool            inext(int8_t &i);
};

#endif // __altcalc_H
