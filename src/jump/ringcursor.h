#ifndef RINGCURSOR_H
#define RINGCURSOR_H

#include <stddef.h>
#include <stdlib.h>

template <class T, const T m_size>
class RingCursor
{
    using value_type = T;

    value_type m_actsize;
    value_type m_cursor;
public:

    RingCursor() :
        m_actsize(1),   // актуальный размер не может быть меньше 1,
                        // т.к. сразу после создания курсор уже указывает на индекс=0,
                        // делая первый раз nxt(), мы получим уже m_actsize=2, а m_cursor=1 - так будет верно,
                        // а если мы при инициализации сделаем m_actsize=0,
                        // то m_actsize будет всегда равен m_cursor (до окончания первого оборота),
                        // т.е. m_actsize будет отставать на 1 от верного значения
        m_cursor(0) {
    }
    
    static value_type size() { return m_size; }
    value_type capacity()   const { return m_actsize; }
    bool       isFirstLoop()const { return m_actsize < m_size; }
    value_type value()      const { return m_cursor; }
    value_type operator*()  const { return m_cursor; }

    // мы можем обращаться к прошедшим индексам,
    // поэтому положительное смещение - это всегда
    // смещение на n позиций назад
    value_type operator[](int n) {
        if (n <= 0)
            return m_cursor;
        if (n >= m_actsize)
            n = m_actsize-1;
        return m_cursor >= n ?
            m_cursor - n :
            m_cursor + m_size - n;
    }
    
    value_type nxt() {
        if (m_actsize < m_size)
            m_actsize++;
        m_cursor ++;
        if (m_cursor >= m_size)
            m_cursor = 0;
        return m_cursor;
    }
    
    RingCursor cur(int n = 0) {
        RingCursor r;
        r.m_actsize = m_actsize;
        r.m_cursor = (*this)[n];
        return r;
    }

    RingCursor first() {
        return cur(m_actsize-1);
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
        if (lhs.m_size != rhs.m_size)
            return 0;
        if (lhs.m_actsize < rhs.m_actsize)
            return 0;
        if (lhs.m_cursor >= rhs.m_cursor)
            return lhs.m_cursor - rhs.m_cursor;
        if (lhs.m_actsize < lhs.m_size)
            return 0;
        return lhs.m_cursor + lhs.size() - rhs.m_cursor;
    }
    RingCursor& operator-=(int n) {
        m_cursor = (*this)[n];
        return *this;
    }
    
    bool comparable(const RingCursor & other) {
        return (m_size == other.size()) && (m_actsize == other.capacity());
    }
    bool operator==(const RingCursor &other) {
        return comparable(other) && (m_cursor == other.m_cursor);
    }
    bool operator!=(const RingCursor &other) {
        return !comparable(other) || (m_cursor != other.m_cursor);
    }
};

#endif // RINGCURSOR_H
