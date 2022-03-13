#ifndef RINGCURSOR_H
#define RINGCURSOR_H

#include <stddef.h>
#include <stdlib.h>

template <class T, const T m_size>
class RingCursor
{
    using value_type = T;

    value_type m_cursor;
public:

    RingCursor() :
        m_cursor(0) {
    }
    
    static value_type size() { return m_size; }
    value_type value() const { return m_cursor; }
    value_type operator*() const { return value(); }
    
    void inc(int n = 1) {
        if (n < 0) {
            dec(-1 * n);
            return;
        }
        
        m_cursor += n;
        m_cursor %= m_size;
    }
    
    void dec(int n = 1) {
        if (n < 0) {
            inc(-1 * n);
            return;
        }

        if (n >= m_size)
            n = m_size-1;
        m_cursor =
            m_cursor >= n ?
                m_cursor - n :
                m_cursor + m_size - n;
    }

    RingCursor& operator++ ()
    {
        inc();
        return *this;
    }
    RingCursor operator ++(int)
    {
        auto cur = *this;
        inc();
        return cur;
    }
    
    RingCursor& operator --()
    {
        dec();
        return *this;
    }
    RingCursor operator --(int) {
        auto cur = *this;
        dec();
        return cur;
    }
    
    friend RingCursor operator+(RingCursor lhs, int rhs) {
        lhs.inc(rhs);
        return lhs;
    }
    friend RingCursor operator+(int lhs, RingCursor rhs) {
        rhs.inc(lhs);
        return rhs;
    }
    RingCursor& operator+=(int n) {
        inc(n);
        return *this;
    }
    
    friend RingCursor operator-(RingCursor lhs, int rhs) {
        lhs.dec(rhs);
        return lhs;
    }
    friend value_type operator-(const RingCursor& lhs, const RingCursor& rhs) {
        return
            lhs.m_cursor >= rhs.m_cursor ?
                lhs.m_cursor - rhs.m_cursor :
                lhs.m_cursor + lhs.size() - rhs.m_cursor;
    }
    RingCursor& operator-=(int n) {
        dec(n);
        return *this;
    }
    
    bool comparable(const RingCursor & other) {
        return (m_size == other.size());
    }
    bool operator==(const RingCursor &other) {
        return comparable(other) && (m_cursor == other.m_cursor);
    }
    bool operator!=(const RingCursor &other) {
        return !comparable(other) || (m_cursor != other.m_cursor);
    }
};

#endif // RINGCURSOR_H
