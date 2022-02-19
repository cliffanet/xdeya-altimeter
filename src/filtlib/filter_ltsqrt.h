#ifndef FILTER_LTSQRT_H
#define FILTER_LTSQRT_H

#include "filter.h"
#include "ring.h"
#include <cmath>

template <class T>
class FilterLtSqrt : public FilterBase<T>
{
    using value_type = T;
    using reference = T & ;
    using const_reference = const T &;
    using size_type = size_t;
    using time_type = uint16_t;

    typedef struct {
        value_type val;
        time_type tm;
    } data_t;

    value_type m_val;
    time_type m_tm;
    value_type m_ka;
    value_type m_kb;
    ring<data_t> m_data;

public:
    FilterLtSqrt(size_type size = 10) :
        m_data(size)
    { }

    bool isvalid()          const { return m_data.size() > 0; }
    const_reference value() const { return m_val; }
    const_reference speed() const { return m_ka; }
    const time_type & tm()  const { return m_tm; }
    const_reference ka()    const { return m_ka; }
    const_reference kb()    const { return m_kb; }

    size_type size()        const { return m_data.size(); }
    size_type capacity()    const { return m_data.capacity(); }
    bool empty()            const { return m_data.empty(); }
    bool full()             const { return m_data.full(); }
    void clear()                  { m_data.clear(); }
    void resize(size_type count)  { m_data.resize(count); }

    void tick(const_reference _val, time_type _tm) {
        m_data.push_back({ _val, _tm });

        // считаем коэффициенты линейной аппроксимации
        // sy - единственный ненулевой элемент в нулевой точке - равен самому первому значению
        value_type
                sy  = m_data.front().val,
                sxy = 0,
                sx  = 0,
                sx2 = 0;
        time_type x = 0;
        for (auto it = m_data.begin()+1; it != m_data.end(); it++) {
            if (it->tm == 0)
                continue;
            x   += it->tm;
            sx  += x;
            sx2 += x * x;
            sy  += it->val;
            sxy += x * it->val;
        }

        m_tm = x;
        if (m_tm > 0) {
            m_ka = (sxy * m_data.size() - (sx * sy)) / (sx2 * m_data.size() - (sx * sx));
            m_kb = (sy - (m_ka * sx)) / m_data.size();
        }
        else {
            m_ka = 0;
            m_kb = _val;
        }
        m_val = m_ka * m_tm + m_kb;
    }
};

#endif // FILTER_LTSQRT_H
