/*
    Workered backend operations functions
*/

#ifndef _sys_H
#define _sys_H

#include "../../def.h"
#include "../core/worker.h"
#include <SD.h>

// воркер операции: позволяет считать процент выполненного,
// видеть текст текущей операции, статус выполненного
#define WRK_OPERATION_STRLEN    64
class WrkOperation : public WrkCmpl {
    public:
        WrkOperation() :
            m_str({ 0 })
            {}

        const char * str() const { return m_str; };

        void str_P(const char *str) {
            strncpy_P(m_str, str, sizeof(m_str));
            m_str[sizeof(m_str)-1] = '\0';
        }
        state_t ok(const char *str) {
            str_P(str);
            return WrkOk::ok();
        }
        state_t err(const char *str) {
            str_P(str);
            return WrkOk::err();
        }

    protected:
        char m_str[WRK_OPERATION_STRLEN];
};

class WrkFwUpdCard : public WrkOperation {
    File fh;

    public:
        WrkFwUpdCard(const char *fname);
        state_t run();
        void end();
};

#endif // _sys_H
