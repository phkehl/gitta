/*!
    \file
    \brief GITTA Tschenggins Lämpli: configuration (see \ref FF_CFG)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"

#include "stuff.h"
#include "debug.h"
#include "status.h"
#include "cfg.h"
#include "json.h"

CFG_MODEL_t  sConfigModel;
CFG_DRIVER_t sConfigDriver;
CFG_ORDER_t  sConfigOrder;
CFG_BRIGHT_t sConfigBright;
CFG_NOISE_t  sConfigNoise;

void cfgInit(void)
{
    INFO("config: init");
    sConfigModel  = CFG_MODEL_UNKNOWN;
    sConfigDriver = CFG_DRIVER_UNKNOWN;
    sConfigOrder  = CFG_ORDER_UNKNOWN;
    sConfigBright = CFG_BRIGHT_UNKNOWN;
    sConfigNoise  = CFG_NOISE_SOME;
}

__INLINE CFG_MODEL_t  cfgGetModel(void)  { return sConfigModel; }
__INLINE CFG_DRIVER_t cfgGetDriver(void) { return sConfigDriver; }
__INLINE CFG_ORDER_t  cfgGetOrder(void)  { return sConfigOrder; }
__INLINE CFG_BRIGHT_t cfgGetBright(void) { return sConfigBright; }
__INLINE CFG_NOISE_t  cfgGetNoise(void)  { return sConfigNoise; }

static const char * const skConfigModelStrs[] =
{
    [CFG_MODEL_UNKNOWN]  = "unknown",
    //[CFG_MODEL_STANDARD] = "standard",
    //[CFG_MODEL_CHEWIE]   = "chewie",
    //[CFG_MODEL_HELLO]    = "hello",
    [CFG_MODEL_GITTA]    = "gitta",
};

static const char * const skConfigDriverStrs[] =
{
    [CFG_DRIVER_UNKNOWN] = "unknown",
    [CFG_DRIVER_NONE]    = "none",
    [CFG_DRIVER_WS2801]  = "WS2801",
    //[CFG_DRIVER_WS2812]  = "WS2812",
    [CFG_DRIVER_SK9822]  = "SK9822",
};

static const char * const skConfigOrderStrs[] =
{
    [CFG_ORDER_UNKNOWN] = "unknown",
    [CFG_ORDER_RGB]     = "RGB",
    [CFG_ORDER_RBG]     = "RBG",
    [CFG_ORDER_GRB]     = "GRB",
    [CFG_ORDER_GBR]     = "GBR",
    [CFG_ORDER_BRG]     = "BRG",
    [CFG_ORDER_BGR]     = "BGR",
};

static const char * const skConfigBrightStrs[] =
{
    [CFG_BRIGHT_UNKNOWN] = "unknown",
    [CFG_BRIGHT_LOW]     = "low",
    [CFG_BRIGHT_MEDIUM]  = "medium",
    [CFG_BRIGHT_HIGH]    = "high",
    [CFG_BRIGHT_FULL]    = "full",
};

static const char * const skConfigNoiseStrs[] =
{
    [CFG_NOISE_UNKNOWN] = "unknown",
    [CFG_NOISE_NONE]    = "none",
    [CFG_NOISE_SOME]    = "some",
    [CFG_NOISE_MORE]    = "more",
    [CFG_NOISE_MOST]    = "most",
};

void cfgMonStatus(void)
{
    DEBUG("mon: config: model=%s driver=%s order=%s bright=%s noise=%s",
        skConfigModelStrs[sConfigModel], skConfigDriverStrs[sConfigDriver],
        skConfigOrderStrs[sConfigOrder], skConfigBrightStrs[sConfigBright],
        skConfigNoiseStrs[sConfigNoise]);
}

static CFG_MODEL_t sConfigStrToModel(const char *str)
{
    if      (strcmp("gitta",    str) == 0) { return CFG_MODEL_GITTA; }
    //else if (strcmp("standard", str) == 0) { return CFG_MODEL_STANDARD; }
    //else if (strcmp("chewie",   str) == 0) { return CFG_MODEL_CHEWIE; }
    //else if (strcmp("hello",    str) == 0) { return CFG_MODEL_HELLO; }
    else                                   { return CFG_MODEL_UNKNOWN; }
}

static CFG_DRIVER_t sConfigStrToDriver(const char *str)
{
    if      (strcmp("none",   str) == 0) { return CFG_DRIVER_NONE; }
    else if (strcmp("WS2801", str) == 0) { return CFG_DRIVER_WS2801; }
    //else if (strcmp("WS2812", str) == 0) { return CFG_DRIVER_WS2812; }
    else if (strcmp("SK9822", str) == 0) { return CFG_DRIVER_SK9822; }
    else                                 { return CFG_DRIVER_UNKNOWN; }
}

static CFG_ORDER_t sConfigStrToOrder(const char *str)
{
    if      (strcmp("RGB",    str) == 0) { return CFG_ORDER_RGB; }
    else if (strcmp("RBG",    str) == 0) { return CFG_ORDER_RBG; }
    else if (strcmp("GRB",    str) == 0) { return CFG_ORDER_GRB; }
    else if (strcmp("GBR",    str) == 0) { return CFG_ORDER_GBR; }
    else if (strcmp("BRG",    str) == 0) { return CFG_ORDER_BRG; }
    else if (strcmp("BGR",    str) == 0) { return CFG_ORDER_BGR; }
    else                                 { return CFG_ORDER_UNKNOWN; }
}

static CFG_BRIGHT_t sConfigStrToBright(const char *str)
{
    if      (strcmp("low",    str) == 0) { return CFG_BRIGHT_LOW; }
    else if (strcmp("medium", str) == 0) { return CFG_BRIGHT_MEDIUM; }
    else if (strcmp("high",   str) == 0) { return CFG_BRIGHT_HIGH; }
    else if (strcmp("full",   str) == 0) { return CFG_BRIGHT_FULL; }
    else                                 { return CFG_BRIGHT_UNKNOWN; }
}

static CFG_NOISE_t sConfigStrToNoise(const char *str)
{
    if      (strcmp("none", str) == 0) { return CFG_NOISE_NONE; }
    else if (strcmp("some", str) == 0) { return CFG_NOISE_SOME; }
    else if (strcmp("more", str) == 0) { return CFG_NOISE_MORE; }
    else if (strcmp("most", str) == 0) { return CFG_NOISE_MOST; }
    else                               { return CFG_NOISE_UNKNOWN; }
}

bool cfgParseJson(char *resp, const int respLen)
{
    DEBUG("config: [%d] %s", respLen, resp);

    const int maxTokens = (4 * 2) + 10;
    jsmntok_t *pTokens = jsmnAllocTokens(maxTokens);
    if (pTokens == NULL)
    {
        ERROR("config: json malloc fail");
        return false;
    }

    // parse JSON response
    const int numTokens = jsmnParse(resp, respLen, pTokens, maxTokens);
    bool okay = numTokens > 0 ? true : false;

    //jsmnDumpTokens(resp, pTokens, numTokens);
    /*
      004.999 D: json 00: 1 obj   000..067 4 <-1 {"driver":"WS2801","model":"standard","noise":"some","order":"RGB"}
      004.999 D: json 01: 3 str   002..008 1 < 0 driver
      004.999 D: json 02: 3 str   011..017 0 < 1 WS2801
      005.010 D: json 03: 3 str   020..025 1 < 0 model
      005.010 D: json 04: 3 str   028..036 0 < 3 standard
      005.020 D: json 05: 3 str   039..044 1 < 0 noise
      005.020 D: json 06: 3 str   047..051 0 < 5 some
      005.020 D: json 07: 3 str   054..059 1 < 0 order
      005.031 D: json 08: 3 str   062..065 0 < 7 RGB
    */

    // process JSON data
    if ( okay && (pTokens[0].type != JSMN_OBJECT) )
    {
        WARNING("backend: json not obj");
        okay = false;
    }

    // look for config key value pairs
    if (okay)
    {
        CFG_MODEL_t  configModel  = CFG_MODEL_UNKNOWN;
        CFG_DRIVER_t configDriver = CFG_DRIVER_UNKNOWN;;
        CFG_ORDER_t  configOrder  = CFG_ORDER_UNKNOWN;
        CFG_BRIGHT_t configBright = CFG_BRIGHT_UNKNOWN;
        CFG_NOISE_t  configNoise  = CFG_NOISE_UNKNOWN;

        for (int ix = 0; ix < (numTokens - 1); ix++)
        {
            const jsmntok_t *pkTok = &pTokens[ix];

            // top-level key
            if (pkTok->parent == 0)
            {
                resp[ pkTok->end ] = '\0';
                const char *key = &resp[ pkTok->start ];
                const jsmntok_t *pkNext = &pTokens[ix + 1];
                if ( (pkNext->parent == ix) && (pkNext->type == JSMN_STRING) )
                {
                    resp[ pkNext->end ] = '\0';
                    const char *val = &resp[ pkNext->start ];
                    //DEBUG("config: %s=%s", key, val);
                    if (strcmp("model",  key) == 0)
                    {
                        configModel  = sConfigStrToModel(val);
                        if (configModel == CFG_MODEL_UNKNOWN)
                        {
                            WARNING("config: illegal model '%s'", val);
                        }
                    }
                    else if (strcmp("driver", key) == 0)
                    {
                        configDriver = sConfigStrToDriver(val);
                        if (configDriver == CFG_DRIVER_UNKNOWN)
                        {
                            WARNING("config: illegal driver '%s'", val);
                        }
                    }
                    else if (strcmp("order", key) == 0)
                    {
                        configOrder  = sConfigStrToOrder(val);
                        if (configOrder == CFG_ORDER_UNKNOWN)
                        {
                            WARNING("config: illegal order '%s'", val);
                        }
                    }
                    else if (strcmp("bright", key) == 0)
                    {
                        configBright = sConfigStrToBright(val);
                        if (configBright == CFG_BRIGHT_UNKNOWN)
                        {
                            WARNING("config: illegal bright '%s'", val);
                        }
                    }
                    else if (strcmp("noise", key) == 0)
                    {
                        configNoise  = sConfigStrToNoise(val);
                        if (configNoise == CFG_NOISE_UNKNOWN)
                        {
                            WARNING("config: illegal noise '%s'", val);
                        }
                    }
                }
            }
        }

        if ( (configModel != CFG_MODEL_UNKNOWN)   &&
             (configDriver != CFG_DRIVER_UNKNOWN) &&
             (configOrder != CFG_ORDER_UNKNOWN)   &&
             (configBright != CFG_BRIGHT_UNKNOWN)   &&
             (configNoise != CFG_NOISE_UNKNOWN) )
        {
            //CS_ENTER; // TODO
            sConfigModel  = configModel;
            sConfigDriver = configDriver;
            sConfigOrder  = configOrder;
            sConfigBright = configBright;
            sConfigNoise  = configNoise;
            //CS_LEAVE;
        }
        else
        {
            ERROR("config: incomplete");
            okay = false;
        }
    }

    // cleanup
    free(pTokens);

    return okay;
}


// eof
