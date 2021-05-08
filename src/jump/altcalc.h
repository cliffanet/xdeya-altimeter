/*
    Alt calculate
*/

#ifndef __altcalc_H
#define __altcalc_H

#include <stdint.h> 

#define AC_LEVEL1_COUNT     10
#define AC_LEVEL2_COUNT     4

// Порог вертикальной скорости, при котором считается, что высота не меняется km/s
#define AC_DIR_SPEED        0.0015
// Коэфициент отклонения A от среднего, при котором отклонение считается слишком сильным (нельзя вычислять состояние)
#define AC_DIR_ERROR        0.1

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
} ac_data1_t;

typedef struct {
    uint32_t interval;
    double ka;
    double kb;
    float altavg;
    float speedavg;
} ac_data2_t;

class AltCalc
{
    public:
        // Текущее давление у земли
        const float         pressgnd()  const { return _pressgnd; }
        // Текущее давление
        const float         press()     const { return cur1 < AC_LEVEL1_COUNT ? _d1[cur1].press : 0; }
        // Текущая высота
        const float         alt()       const { return cur1 < AC_LEVEL1_COUNT ? _d1[cur1].alt : 0; }
        // Среднее значение высоты за AC_LEVEL1_COUNT циклов
        const float         altavg()    const { return _d2[cur2].altavg; }
        // Высота по аппроксимации
        const float         altapp()    const { return _d2[cur2].ka * _d2[cur2].interval + _d2[cur2].kb; }
        // Средняя скорость за AC_LEVEL1_COUNT циклов
        const float         speedavg()  const { return _d2[cur2].speedavg; }
        // Скорость по аппроксимации м/с
        const float         speedapp()  const { return _d2[cur2].ka*1000; }
        // Текущий режим высоты (определяется автоматически)
        const ac_state_t    state()     const { return _state; }
        // Время с предыдущего изменения режима высоты
        const uint32_t      statetm()   const { return _statetm; }
        void statetmreset() { _statetm = 0; }
        // Направление вертикального движения (вверх/вниз)
        const ac_direct_t   direct()    const { return _dir; }
        // Как долго сохраняется текущее направления движения (в ms)
        const uint32_t      dirtm()     const { return _dirtm; }
        void dirtmreset() { _dirtm = 0; }
        
        // поля для отладки коэфициентов
        // среднее значение коэфициента A на всём протяжении _d2
        const double        kaavg()     const { return _kaavg; }
        // максимальное отклонение коэфициента А от среднего значения в процентах
        const double        adiffmax()  const { return _adiffmax; }
        
        // очередное полученное значение давления и интервал в ms после предыдущего вычисления
        void tick(float press, uint16_t interval);
        // пересчитать из данных LEVEL-1 коэфициенты линейной аппроксимации и средние значения
        void l1calc();
        // переключиться на следующую ячейку LEVEL-2 (нужно делать после каждого полного цикла level-1)
        void l2next();
        // обновляет текущее состояние, вычисленное по LEVEL-2, выполняется при каждом l2add()
        // однако, если состояние ACST_INIT, то оно не будет меняться, пока не заполнится весь LEVEL-2
        ac_state_t stateupdate();
        // сбрасывает "ноль" высоты в текущие показания, а так же обнуляет все состояния
        void gndreset();
  
    private:
        float _pressgnd = 101325;
        ac_data1_t _d1[AC_LEVEL1_COUNT];
        ac_data2_t _d2[AC_LEVEL2_COUNT];
        uint8_t cur1 = 0xff;
        uint8_t cur2 = 0;
        ac_state_t _state = ACST_INIT;
        ac_direct_t _dir = ACDIR_INIT;
        uint32_t _dirtm = 0;
        uint32_t _statetm = 0;
        double _kaavg = 0, _adiffmax = 0;
};

#endif // __altcalc_H
