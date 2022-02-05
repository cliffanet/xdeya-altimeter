#ifndef FILTER_BASE_H
#define FILTER_BASE_H

#include <stdint.h>

template <class T>
class FilterBase
{
    using value_type = T;
    using reference = T & ;
    using const_reference = const T &;

public:
    virtual void tick(const_reference val, uint16_t tm) = 0;
    virtual const_reference value() const = 0;
    virtual const_reference speed() const = 0;
};

#endif // FILTER_BASE_H
