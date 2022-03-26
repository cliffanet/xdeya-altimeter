/*
    View: Text lang
*/

#ifndef _view_text_l_H
#define _view_text_l_H

#include "../../def.h"

#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
#include "text.ru.h"
#else
#include "text.en.h"
#endif

#endif // _view_text_l_H
