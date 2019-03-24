/*!
    \file
    \brief GITTA Tschenggins Lämpli: system monitor (see \ref FF_MON)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup FF_MON MON
    \ingroup FF

    @{
*/
#ifndef __MON_H__
#define __MON_H__

#include "stdinc.h"

//! start system monitor
void monStart(void);

void monIsrEnter(void);
void monIsrLeave(void);


//! monitor function type
typedef void (*MON_FUNC_t)(void);

//! register function to be called on every monitor period
bool monRegisterMonFunc(MON_FUNC_t monFunc);

//! set monitor period [s] (0 = monitor off)
void monSetPeriod(const char *period);


#endif // __MON_H__
//@}
