/*
    Alt calculate
*/

#ifndef __altcalc_H
#define __altcalc_H

#include <stdint.h> 

#define AC_DATA_COUNT       30

// Порог скорости для режима FLAT (abs, m/s)
// Он же является порогом удержания для режима canopy
#define AC_SPEED_FLAT       1.5
// Порог срабатывания режима canopy,
// т.е. если скорость снижения выше этой, то canopy
// ну или freefall
#define AC_SPEED_CANOPY_I     4
// Порог срабатывания режима freefall безусловный (abs, m/s)
// т.е. если скорость снижения выше этой, то полюбому - ff
#define AC_SPEED_FREEFALL_I   30
// Порог удержания режима freefall,
// т.е. если текущий режим freefall, 
// то он будет удерживаться таким, пока скорость снижения выше этой
#define AC_SPEED_FREEFALL_O   20

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
        
        // среднеквадратическое отклонение прямой скорости
        const double        sqdiff()  const { return _sqdiff; }
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
        ac_state_t stateupdate();
        // сбрасывает "ноль" высоты в текущие показания, а так же обнуляет все состояния
        void gndreset();
  
    private:
        float _pressgnd = 101325;
        ac_data_t _data[AC_DATA_COUNT];
        uint8_t _c = 0xff;
        double _ka = 0, _kb = 0, _sqdiff = 0;
        float _alt0 = 0, _press0 = 0, _altavg = 0, _speedavg = 0;
        uint32_t _interval = 0;
        ac_state_t _state = ACST_INIT;
        ac_direct_t _dir = ACDIR_INIT;
        uint32_t _dircnt = 0, _dirtm = 0;
        uint32_t _statecnt = 0, _statetm = 0;
        
        int8_t          i2i(int8_t i);
        const int8_t    ifrst() const;
        bool            inext(int8_t &i);
};

#endif // __altcalc_H
