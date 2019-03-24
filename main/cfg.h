/*!
    \file
    \brief GITTA Tschenggins Lämpli: configuration (see \ref FF_CFG)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup FF_CFG CFG
    \ingroup FF

    @{
*/
#ifndef __CFG_H__
#define __CFG_H__

#include "stdinc.h"

//! initialise
void cfgInit(void);

void cfgMonStatus(void);

typedef enum CFG_MODEL_e
{
    CFG_MODEL_UNKNOWN,
    //CFG_MODEL_STANDARD,
    //CFG_MODEL_CHEWIE,
    //CFG_MODEL_HELLO,
    CFG_MODEL_GITTA,
} CFG_MODEL_t;

typedef enum CFG_DRIVER_e
{
    CFG_DRIVER_UNKNOWN,
    CFG_DRIVER_NONE,
    CFG_DRIVER_WS2801,
    //CFG_DRIVER_WS2812,
    CFG_DRIVER_SK9822,
} CFG_DRIVER_t;

typedef enum CFG_ORDER_e
{
    CFG_ORDER_UNKNOWN,
    CFG_ORDER_RGB,
    CFG_ORDER_RBG,
    CFG_ORDER_GRB,
    CFG_ORDER_GBR,
    CFG_ORDER_BRG,
    CFG_ORDER_BGR,
} CFG_ORDER_t;

typedef enum CFG_BRIGHT_e
{
    CFG_BRIGHT_UNKNOWN,
    CFG_BRIGHT_LOW,
    CFG_BRIGHT_MEDIUM,
    CFG_BRIGHT_HIGH,
    CFG_BRIGHT_FULL,
} CFG_BRIGHT_t;

typedef enum CFG_NOISE_e
{
    CFG_NOISE_UNKNOWN,
    CFG_NOISE_NONE,
    CFG_NOISE_SOME,
    CFG_NOISE_MORE,
    CFG_NOISE_MOST,
} CFG_NOISE_t;

CFG_MODEL_t  cfgGetModel(void);
CFG_DRIVER_t cfgGetDriver(void);
CFG_ORDER_t  cfgGetOrder(void);
CFG_BRIGHT_t cfgGetBright(void);
CFG_NOISE_t  cfgGetNoise(void);

bool cfgParseJson(char *resp, const int respLen);


#endif // __CFG_H__
//@}
// eof
