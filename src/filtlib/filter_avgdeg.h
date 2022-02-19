#ifndef FILTER_AVG_DEG_H
#define FILTER_AVG_DEG_H

#include "filter.h"
#include "ring.h"
#include <cmath>

template <class T>
class FilterAvgDeg : public FilterBase<T>
{
    using value_type = T;
    using reference = T & ;
    using const_reference = const T &;
    using size_type = size_t;
    using time_type = uint16_t;

    typedef struct {
        value_type val;
        value_type vsin;
        value_type vcos;
        time_type tm;
    } data_t;

    value_type m_val;
    value_type m_speed;
    ring<data_t> m_data;

public:
    FilterAvgDeg(size_type size = 10) :
        m_data(size)
    { }

    bool isvalid()          const { return m_data.size() > 0; }
    const_reference value() const { return m_val; }
    const_reference speed() const { return m_speed; }

    size_type size()        const { return m_data.size(); }
    size_type capacity()    const { return m_data.capacity(); }
    bool empty()            const { return m_data.empty(); }
    bool full()             const { return m_data.full(); }
    void clear()                  { m_data.clear(); }
    void resize(size_type count)  { m_data.resize(count); }

    void tick(const_reference _val, time_type _tm) {
        m_data.push_back({ _val, sin(_val), cos(_val), _tm });

        value_type ssin = 0;
        value_type scos = 0;
        time_type tm = 0;
        for (auto it = m_data.begin()+1; it != m_data.end(); it++) {
            ssin += it->vsin;
            scos += it->vcos;
            tm += it->tm;
        }

        m_val = atan2(ssin, scos);

        if (tm > 0) {
            value_type d = m_data.back().val - m_data.front().val;
            while ((d > 0) && (d > M_PI))
                d -= 2*M_PI;
            while ((d < 0) && (d < -M_PI))
                d += 2*M_PI;
            if ((d < 0) && (-1*d > d+M_PI))
                d += M_PI;
            if ((d > 0) && (d > -1*(d-M_PI)))
                d -= M_PI;
            m_speed = d / tm;
        }
        else
            m_speed = 0;
    }
};

#endif // FILTER_AVG_DEG_H
