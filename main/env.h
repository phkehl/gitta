/*!
    \file
    \brief GITTA Tschenggins Lämpli: environment (see \ref FF_ENV)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup FF_MON MON
    \ingroup FF

    @{
*/
#ifndef __ENV_H__
#define __ENV_H__

#include "stdinc.h"

//! initialise env
void envInit(void);

//! get enviroment variable
const char *envGet(const char *key);

//! set environment variable
bool envSet(const char *key, const char *val);

//! get environment variable default
const char *envDefault(const char *key);

//! print environment variables
void envPrint(const char *prefix);

#endif // __ENV_H__
//@}
