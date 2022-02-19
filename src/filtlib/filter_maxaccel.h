#ifndef FILTER_MAXACCEL_H
#define FILTER_MAXACCEL_H

#include "filter.h"

template <class T>
class FilterMaxAccel : public FilterBase<T>
{
    using value_type = T;
    using reference = T & ;
    using const_reference = const T &;
    using time_type = uint16_t;

    value_type m_val;
    value_type m_speed;
    value_type m_accel;
    value_type m_acc2;
    value_type m_max_speed;
    value_type m_max_accel;
    value_type m_max_acc2;
    bool m_valid;

public:
    FilterMaxAccel(value_type maxspeed = 1, value_type maxaccel = 1, value_type maxacc2 = 1) :
        m_val(0),
        m_speed(0),
        m_accel(0),
        m_acc2(0),
        m_max_speed(maxspeed),
        m_max_accel(maxaccel),
        m_max_acc2(maxacc2),
        m_valid(false)
    { }

    bool isvalid()          const { return m_valid; }
    const_reference value() const { return m_val; }
    const_reference speed() const { return m_speed; }
    const_reference accel() const { return m_accel; }
    const_reference acc2()  const { return m_acc2; }

    const_reference max_speed() const { return m_max_speed; }
    const_reference max_accel() const { return m_max_accel; }
    const_reference max_acc2()  const { return m_max_acc2; }
    void set_max_speed(const_reference val) { m_max_speed = val; }
    void set_max_accel(const_reference val) { m_max_accel = val; }
    void set_max_acc2(const_reference val)  { m_max_acc2 = val; }

    void clear() {
        m_val = 0;
        m_speed = 0;
        m_accel = 0;
        m_acc2 = 0;
        m_valid = false;
    }

    void tick(const_reference val, time_type tm) {
        if (!m_valid) {
            m_val = val;
            m_valid = true;
            return;
        }

        if (tm <= 0) {
            m_val = (m_val + val) / 2;
            return;
        }

        value_type _val = val;
        value_type sp = (val-m_val) / tm;
        if (bymax(sp, m_max_speed)) {
            _val = sp * tm + m_val;
        }
        value_type acc = (sp-m_speed) / tm;
        if (bymax(acc, m_max_accel)) {
            sp = acc * tm + m_speed;
            _val = sp * tm + m_val;
        }
        value_type acc2 = (acc-m_acc2) / tm;
        if (bymax(acc2, m_max_acc2)) {
            acc = acc2 * tm + m_acc2;
            sp = acc * tm + m_speed;
            _val = sp * tm + m_val;
        }

        m_val = _val;
        m_speed = sp;
        m_accel = acc;
        m_acc2 = acc2;
    }
private:
    inline bool bymax(reference val, const_reference max) {
        if (abs(val) <= abs(max))
            return false;
        val = val >= 0 ? abs(max) : -1*abs(max);
        return true;
    }
};

#endif // FILTER_MAXACCEL_H
