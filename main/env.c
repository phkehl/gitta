/*
    \file
    \brief GITTA Tschenggins Lämpli: environment (see \ref FF_ENV)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"

#include <nvs.h>
#include <nvs_flash.h>

#include "debug.h"
#include "stuff.h"
#include "env.h"


/* *********************************************************************************************** */

typedef struct ENV_REG_s
{
    const char  *key;
    char        *val;
    const size_t size;
    const char  *def;
} ENV_REG_t;

static char sEnvSsid[50];
static char sEnvPass[50];
static char sEnvDebug[10];
static char sEnvMonitor[10];
static char sEnvBackend[CONFIG_FF_BACKEND_URL_MAXLEN];

static const ENV_REG_t skEnvReg[] =
{
    { .key = "ssid",    .val = sEnvSsid,    .size = sizeof(sEnvSsid),    .def = CONFIG_FF_ENV_SSID_DEFAULT    },
    { .key = "pass",    .val = sEnvPass,    .size = sizeof(sEnvPass),    .def = CONFIG_FF_ENV_PASS_DEFAULT    },
    { .key = "debug",   .val = sEnvDebug,   .size = sizeof(sEnvDebug),   .def = CONFIG_FF_ENV_DEBUG_DEFAULT   },
    { .key = "monitor", .val = sEnvMonitor, .size = sizeof(sEnvMonitor), .def = CONFIG_FF_ENV_MONITOR_DEFAULT },
    { .key = "backend", .val = sEnvBackend, .size = sizeof(sEnvBackend), .def = CONFIG_FF_ENV_BACKEND_DEFAULT },
};

// -------------------------------------------------------------------------------------------------

static nvs_handle sEnvNvsHandle;

static bool sEnvStoreStr(const char *key, const char *val)
{
    bool res = true;
    esp_err_t err = nvs_set_str(sEnvNvsHandle, key, val);
    DEBUG("sEnvStoreStr(%s, \"%s\"): %s", key, val, esp_err_to_name(err));
    switch (err)
    {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_READ_ONLY:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
        case ESP_ERR_NVS_REMOVE_FAILED:
        case ESP_ERR_NVS_VALUE_TOO_LONG:
        default:
            WARNING("nvs: failed storing key '%s': %s", key, esp_err_to_name(err));
            res = false;
            break;
    }
    return res;
}

static bool sEnvLoadStr(const char *key, char *buf, const size_t size)
{
    bool res = true;
    size_t reqLen;
    esp_err_t err = nvs_get_str(sEnvNvsHandle, key, NULL, &reqLen);
    DEBUG("sEnvLoadStr(%s, %p, %u): reqLen=%u, err=%s", key, buf, size, reqLen, esp_err_to_name(err));
    switch (err)
    {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            //if (sEnvStoreStr(key, ""))
            //{
            //    reqLen = strlen(def) + 1;
            //}
            //else
            {
                res = false;
            }
            break;
        case ESP_ERR_NVS_INVALID_HANDLE:
        case ESP_ERR_NVS_INVALID_NAME:
        case ESP_ERR_NVS_INVALID_LENGTH:
        default:
            WARNING("nvs: failed loading key '%s': %s", key, esp_err_to_name(err));
            res = false;
            break;
    }

    if (res)
    {
        if (reqLen > size)
        {
            WARNING("nvs: buf too small (%u > %u) for key '%s'", reqLen, size, key);
            res = false;
        }
        else
        {
            esp_err_t err2 = nvs_get_str(sEnvNvsHandle, key, buf, &reqLen);
            if (err2 != ESP_OK)
            {
                WARNING("nvs: failed loading key '%s' after init: %s", key, esp_err_to_name(err));
                res = false;
            }
            //else
            //{
            //    DEBUG("nvs: loaded %s=\"%s\"", key, buf);
            //}
        }
    }

    return res;
}

// -------------------------------------------------------------------------------------------------

const char *envGet(const char *key)
{
    const char *val = NULL;
    for (int ix = 0; ix < (int)NUMOF(skEnvReg); ix++)
    {
        const ENV_REG_t *pkReg = &skEnvReg[ix];
        if (strcmp(pkReg->key, key) == 0)
        {
            val = pkReg->val;
            break;
        }
    }
    return val;
}

bool envSet(const char *key, const char *val)
{
    bool ret = false;
    for (int ix = 0; ix < (int)NUMOF(skEnvReg); ix++)
    {
        const ENV_REG_t *pkReg = &skEnvReg[ix];
        if (strcmp(pkReg->key, key) == 0)
        {
            if ( (strlen(val) < pkReg->size) && sEnvStoreStr(key, val) )
            {
                strncpy(pkReg->val, val, pkReg->size);
                ret = true;
            }
            break;
        }
    }
    return ret;
}

const char *envDefault(const char *key)
{
    const char *val = NULL;
    for (int ix = 0; ix < (int)NUMOF(skEnvReg); ix++)
    {
        const ENV_REG_t *pkReg = &skEnvReg[ix];
        if (strcmp(pkReg->key, key) == 0)
        {
            val = pkReg->def;
            break;
        }
    }
    return val;
}

// -------------------------------------------------------------------------------------------------

void envPrint(const char *prefix)
{
    for (int ix = 0; ix < (int)NUMOF(skEnvReg); ix++)
    {
        const ENV_REG_t *pkReg = &skEnvReg[ix];
        if (prefix == NULL)
        {
            DEBUG("env: %s \"%s\"", pkReg->key, pkReg->val);
        }
        else
        {
            PRINT("%s %s \"%s\"", prefix, pkReg->key, pkReg->val);
        }
    }
}

// -------------------------------------------------------------------------------------------------

void envInit(void)
{
    INFO("env: init");

    // initialise NVS
    {
        esp_err_t err = nvs_flash_init();
        if ( (err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
        {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
    }

    // open our own NVS namespace
    {
        esp_err_t err = nvs_open(CONFIG_FF_NAME, NVS_READWRITE, &sEnvNvsHandle);
        ESP_ERROR_CHECK(err);

        nvs_stats_t nvsStats;
        nvs_get_stats(NULL, &nvsStats);
        DEBUG("nvs: ns=%u total=%u used=%u free=%u",
            nvsStats.namespace_count, nvsStats.total_entries, nvsStats.used_entries, nvsStats.free_entries);
    }

    // load all environment variables, save defaults if it doesn't exist yet
    for (int ix = 0; ix < (int)NUMOF(skEnvReg); ix++)
    {
        const ENV_REG_t *pkReg = &skEnvReg[ix];
        if (!sEnvLoadStr(pkReg->key, pkReg->val, pkReg->size))
        {
            strncpy(pkReg->val, pkReg->def, pkReg->size);
            ESP_ERROR_CHECK( sEnvStoreStr(pkReg->key, pkReg->val) ? ESP_OK : ESP_FAIL );
        }
    }

    INFO("env: ssid=%s pass=%s debug=%s monitor=%s backend=%s",
        strlen(sEnvSsid) > 0 ? sEnvSsid : "<empty>",
        strlen(sEnvPass) > 0 ? "***"     : "<empty>",
        sEnvDebug, sEnvMonitor,
        strlen(sEnvBackend) > 0 ? sEnvBackend : "<empty>");
}

/* *********************************************************************************************** */
// eof
