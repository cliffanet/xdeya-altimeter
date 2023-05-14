#ifndef RINGCURSOR_H
#define RINGCURSOR_H

#include <stddef.h>
#include <stdlib.h>

template <class T, const T m_max>
class RingCursor
{
    using value_type = T;

    value_type m_size;
    value_type m_cursor;
public:

    RingCursor() :
        m_size(1),      // актуальный размер не может быть меньше 1,
                        // т.к. сразу после создания курсор уже указывает на индекс=0,
                        // делая первый раз nxt(), мы получим уже m_size=2, а m_cursor=1 - так будет верно,
                        // а если мы при инициализации сделаем m_size=0,
                        // то m_size будет всегда равен m_cursor (до окончания первого оборота),
                        // т.е. m_size будет отставать на 1 от верного значения
        m_cursor(0) {
    }
    
    static value_type capacity()  { return m_max; }
    value_type size()       const { return m_size; }
    bool       full()       const { return m_size >= m_max; }
    value_type value()      const { return m_cursor; }
    value_type operator*()  const { return m_cursor; }

    // мы можем обращаться к прошедшим индексам,
    // поэтому положительное смещение - это всегда
    // смещение на n позиций назад
    value_type operator[](int n) {
        if (n <= 0)
            return m_cursor;
        if (n >= m_size)
            n = m_size-1;
        return m_cursor >= n ?
            m_cursor - n :
            m_cursor + m_max - n;
    }
    
    value_type nxt() {
        if (m_size < m_max)
            m_size++;
        m_cursor ++;
        if (m_cursor >= m_max)
            m_cursor = 0;
        return m_cursor;
    }
    
    RingCursor cur(int n = 0) {
        RingCursor r;
        r.m_size = m_size;
        r.m_cursor = (*this)[n];
        return r;
    }

    RingCursor first() {
        return cur(m_size-1);
    }

    RingCursor& operator++ () {
        nxt();
        return *this;
    }
    RingCursor operator ++(int)
    {
        auto prv = *this;
        nxt();
        return prv;
    }
    
    friend RingCursor operator-(RingCursor lhs, int n) {
        return lhs.cur(n);
    }
    friend value_type operator-(const RingCursor& lhs, const RingCursor& rhs) {
        if (lhs.m_max != rhs.m_max)
            return 0;
        if (lhs.m_size < rhs.m_size)
            return 0;
        if (lhs.m_cursor >= rhs.m_cursor)
            return lhs.m_cursor - rhs.m_cursor;
        if (lhs.m_size < lhs.m_max)
            return 0;
        return lhs.m_cursor + lhs.size() - rhs.m_cursor;
    }
    RingCursor& operator-=(int n) {
        m_cursor = (*this)[n];
        return *this;
    }
    
    bool comparable(const RingCursor & other) {
        return (m_max == other.capacity()) && (m_size == other.m_size);
    }
    bool operator==(const RingCursor &other) {
        return comparable(other) && (m_cursor == other.m_cursor);
    }
    bool operator!=(const RingCursor &other) {
        return !comparable(other) || (m_cursor != other.m_cursor);
    }
};

#endif // RINGCURSOR_H
