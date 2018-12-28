/*
    \file
    \brief GITTA Tschenggins Lämpli: debugging (see \ref FF_DEBUG)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"
#include "stuff.h"
#include "debug.h"

static SemaphoreHandle_t sDebugMutex;

void debugInit(void)
{
    // must initialise first, before we can use any DEBUG() etc.
    static StaticSemaphore_t sMutex;
    sDebugMutex = xSemaphoreCreateMutexStatic(&sMutex);

    INFO("debug: init");

    esp_log_level_set(CONFIG_FF_NAME, ESP_LOG_DEBUG);
}

bool debugSetLevel(const char *level)
{
    const char *tags[] =
    {
        "HTTP_CLIENT", "nvs", "memory_layout", "heap_init", "event", "tcpip_adapter", "esp-tls",
        "wifi", "intr_alloc", "phy_init", "RTC_MODULE", "spi_master",
    };
    if (level != NULL)
    {
        if (strcmp(level, "on") == 0)
        {
            DEBUG("debug: on");
            for (int ix = 0; ix < (int)NUMOF(tags); ix++)
            {
                esp_log_level_set(tags[ix], ESP_LOG_DEBUG);
            }
            return true;
        }
        else if (strcmp(level, "off") == 0)
        {
            DEBUG("debug: off");
            for (int ix = 0; ix < (int)NUMOF(tags); ix++)
            {
                esp_log_level_set(tags[ix], ESP_LOG_INFO);
            }
            return true;
        }
    }
    return false;
}


__INLINE void debugLock(void)
{
    xSemaphoreTake(sDebugMutex, portMAX_DELAY);
}

__INLINE void debugUnlock(void)
{
    xSemaphoreGive(sDebugMutex);
}

void debugAssert(const char *file, const int line, const char *func, const char *expr)
{
    const char *pAddr =  __builtin_return_address(0);
    ERROR("Assertion failed: %s:%d:%s() at %p --  %s", file, line, func,
        pAddr - 3, expr);
    FLUSH();
    osSleep(2000);
    esp_restart();
}


// eof
