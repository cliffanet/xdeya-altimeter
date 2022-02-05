#ifndef FILTER_AVG_H
#define FILTER_AVG_H

#include "filter.h"
#include "ring.h"
#include <cmath>

template <class T>
class FilterAvg : public FilterBase<T>
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
    value_type m_speed;
    ring<data_t> m_data;

public:
    FilterAvg(size_type size = 10) :
        m_data(size)
    { }

    const_reference value() const { return m_val; }
    const_reference speed() const { return m_speed; }

    size_type size()        const { return m_data.size(); }
    size_type capacity()    const { return m_data.capacity(); }
    bool empty()            const { return m_data.empty(); }
    bool full()             const { return m_data.full(); }
    void clear()                  { m_data.clear(); }
    void resize(size_type count)  { m_data.resize(count); }

    void tick(const_reference _val, time_type _tm) {
        m_data.push_back({ _val, _tm });

        value_type val = m_data.front().val;
        time_type tm = 0;
        for (auto it = m_data.begin()+1; it != m_data.end(); it++) {
            val += it->val;
            tm += it->tm;
        }

        m_val = std::round(val / m_data.size());
        m_speed = tm > 0 ? (m_data.back().val - m_data.front().val) / tm : 0;
    }
};

#endif // FILTER_AVG_H
