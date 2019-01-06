/*
    \file
    \brief GITTA Tschenggins Lämpli: debugging (see \ref FF_DEBUG)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup FF_DEBUG DEBUG
    \ingroup FF

    @{
*/
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "stdinc.h"


//! initialise debug
void debugInit(void);

//! set debug level ("on" or "off")
bool debugSetLevel(const char *level);

void debugLock(void);
void debugUnlock(void);

//! print an error (#ESP_LOG_ERROR) message \hideinitializer
#define ERROR(fmt, ...)   debugLock(); ESP_LOGE(CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()

//! print a warning (#ESP_LOG_WARN) message \hideinitializer
#define WARNING(fmt, ...) debugLock(); ESP_LOGW(CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()

//! print an info (#ESP_LOG_INFO) message \hideinitializer
#define INFO(fmt, ...)   debugLock(); ESP_LOGI(CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()

//! print a debug (#ESP_LOG_DEBUG) message \hideinitializer
//#define DEBUG(fmt, ...)   debugLock(); ESP_LOGD(CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()
#define DEBUG(fmt, ...)   debugLock(); ESP_LOG_LEVEL(ESP_LOG_DEBUG, CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()
// (always compile in our debug stuff, even if CONFIG_LOG_DEFAULT_LEVEL wouldn't allow it)

//! print a trace (#ESP_LOG_VERBOSE) message \hideinitializer
#define TRACE(fmt, ...)   debugLock(); ESP_LOGV(CONFIG_FF_NAME, fmt, ## __VA_ARGS__); debugUnlock()

//! hex dump data (#ESP_LOG_DEBUG) message \hideinitializer
#define HEXDUMP(ptr, size) ESP_LOG_BUFFER_HEXDUMP(CONFIG_FF_NAME, ptr, size, ESP_LOG_DEBUG);

//! print a normal non-log string \hideinitializer
#define PRINT(fmt, ...) debugLock(); printf(fmt "\n", ## __VA_ARGS__); debugUnlock()

//! flush all output
#define FLUSH() fflush(stdout)

void debugAssert(const esp_err_t err, const char *func, const char *expr);

#define ASSERT(expr) (expr) ? (void)0 : debugAssert(ESP_OK, __FUNCTION__, #expr)

#define ASSERT_ESP_OK(expr) do { const esp_err_t __err = (expr); if (__err != ESP_OK) { debugAssert(__err, __FUNCTION__, #expr); } } while (0)



// (intptr_t)__builtin_return_address(0) - 3
#endif // __DEBUG_H__
//@}
