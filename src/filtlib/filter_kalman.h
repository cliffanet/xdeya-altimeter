#ifndef FILTER_KALMAN_H
#define FILTER_KALMAN_H

#include "filter.h"

template <class T>
class FilterKalman : public FilterBase<T>
{
    using value_type = T;
    using reference = T & ;
    using const_reference = const T &;
    using time_type = uint16_t;

    value_type m_val;
    value_type m_speed;
    value_type m_q;
    value_type m_r;
    value_type m_f;
    value_type m_h;
    value_type m_covar;
    bool m_valid;

public:
    FilterKalman(value_type q = 2, value_type r = 15, value_type f = 1, value_type h = 1, value_type covar = 0.1) :
        m_val(0),
        m_speed(0),
        m_q(q),
        m_r(r),
        m_f(f),
        m_h(h),
        m_covar(covar),
        m_valid(false)
    { }

    bool isvalid()          const { return m_valid; }
    const_reference value() const { return m_val; }
    const_reference speed() const { return m_speed; }
    const_reference q()     const { return m_q; }
    const_reference r()     const { return m_r; }
    const_reference f()     const { return m_f; }
    const_reference h()     const { return m_h; }
    const_reference covar() const { return m_covar; }

    void set_q(const_reference val) { m_q = val; }
    void set_r(const_reference val) { m_r = val; }
    void set_f(const_reference val) { m_f = val; }
    void set_h(const_reference val) { m_h = val; }

    void set_covar(const_reference val) { m_covar = val; }

    void clear() {
        m_val = 0;
        m_speed = 0;
        m_valid = false;
    }

    void tick(const_reference val, time_type tm) {
        if (!m_valid) {
            m_val = val;
            m_valid = true;
            return;
        }

        // time update - prediction
        value_type x0 = m_f * m_val;
        value_type p0 = m_f * m_f * m_covar + m_q;

        // measurement update - correction
        value_type K = m_h * p0 / (m_h * m_h * p0 + m_r);
        value_type v = x0 + K * (val - m_h * x0);
        m_covar = (1 - K * m_h) * p0;

        // save
        m_speed = (v-m_val) / tm;
        m_val = v;
    }
};

#endif // FILTER_KALMAN_H
