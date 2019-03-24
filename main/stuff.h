/*!
    \file
    \brief GITTA Tschenggins Lämpli: stuff (see \ref FF_STUFF)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup FF_STUFF STUFF
    \ingroup FF

    @{
*/
#ifndef __STUFF_H__
#define __STUFF_H__

#include "stdinc.h"


//! initialise stuff
void stuffInit(void);

/* ***** handy macros **************************************************************************** */

/*!
    \name Handy Macros
    @{
*/

//#define BIT(bit) (1<<(bit))   //!< bit
#ifndef UNUSED
#  define UNUSED(foo) (void)foo //!< unused variable
#endif
#ifndef NULL
#  define NULL (void *)0    //!< null pointer     \hideinitializer
#endif /* NULL */
#define NUMOF(x) (sizeof(x)/sizeof(*(x)))       //!< number of elements in vector     \hideinitializer
#define ENDLESS true          //!< for endless while loops     \hideinitializer
#define FALLTHROUGH           //!< switch fall-through marker     \hideinitializer
#define __PAD(n) uint8_t __PADNAME(__LINE__)[n]  //!< struct padding macro     \hideinitializer
#define __PADFILL { 0 }           //!< to fill const padding     \hideinitializer
#define MIN(a, b)  ((b) < (a) ? (b) : (a)) //!< smaller value of a and b     \hideinitializer
#define MAX(a, b)  ((b) > (a) ? (b) : (a)) //!< bigger value of a and b     \hideinitializer
#define ABS(a) ((a) > 0 ? (a) : -(a)) //!< absolute value     \hideinitializer
#define CLIP(x, a, b) ((x) <= (a) ? (a) : ((x) >= (b) ? (b) : (x))) //!< clip value in range [a:b]     \hideinitializer
#define STRINGIFY(x) _STRINGIFY(x) //!< stringify argument     \hideinitializer
#define CONCAT(a, b) _CONCAT(a, b) //!< concatenate arguments     \hideinitializer
#define SWAP2(x) ( (( (x) >>  8)                                                             | ( (x) <<  8)) )
#define SWAP4(x) ( (( (x) >> 24) | (( (x) & 0x00FF0000) >>  8) | (( (x) & 0x0000FF00) <<  8) | ( (x) << 24)) )
//@}

#ifndef __DOXYGEN__
#  define _STRINGIFY(x) #x
#  define _CONCAT(a, b)  a ## b
#  define ___PADNAME(x) __pad##x
#  define __PADNAME(x) ___PADNAME(x)
#endif

//! \name Compiler Hints etc.
//@{
#define __PURE()              __attribute__ ((pure))          //!< pure \hideinitializer
#define __IRQ()               __attribute__ ((interrupt))     //!< irq \hideinitializer
#define __WEAK()              __attribute__ ((weak))          //!< weak \hideinitializer
#define __PACKED              __attribute__ ((packed))        //!< packed \hideinitializer
#define __ALIGN(n)            __attribute__ ((aligned (n)))   //!< align \hideinitializer
#ifdef __INLINE
#  undef __INLINE
#endif
#define __INLINE              inline                                 //!< inline \hideinitializer
#define __NOINLINE            __attribute__((noinline))              //!< no inline \hideinitializer
#define __FORCEINLINE         __attribute__((always_inline)) inline  //!< force inline (also with -Os) \hideinitializer
#define __USED                __attribute__((used))                  //!< used \hideinitializer
#define __NORETURN            __attribute__((noreturn))              //!< no return \hideinitializer
#define __PRINTF(six, aix)    __attribute__((format(printf, six, aix))) //!< printf() style func \hideinitializer
#define __SECTION(sec)        __attribute__((section (STRINGIFY(sec)))); //!< place symbol in section \hideinitializer
#define __NAKED               __attribute__((naked))                 //!< naked function \hideinitializer
//@}

/* ***** OS helpers ****************************************************************************** */

/*!
    \name OS helpers
    @{
*/

//! enter a critical section, asserting that interrupts are off \hideinitializer
#define CS_ENTER do { taskENTER_CRITICAL();

//! leave a critical section, re-enabling the interrupts if necessary \hideinitializer
#define CS_LEAVE taskEXIT_CRITICAL(); } while (0)

//! convert ms to ticks
#define MS2TICKS(ms) ((ms) / portTICK_PERIOD_MS)

//! convert ticks to ms
#define TICKS2MS(ticks) ((ticks) * portTICK_PERIOD_MS)

//! sleep
#define osSleep(ms)  vTaskDelay(MS2TICKS(ms))

//! get time
#define osTime() (TICKS2MS(xTaskGetTickCount()))

//! set POSIX time
void osSetPosixTime(const uint32_t timestamp);

//! get POSIX time
uint32_t osGetPosixTime(void);

//@}

/* ***** IDF enumeration stringifications ******************************************************** */

/*!
    \name IDF Enum Stringification
    @{
*/

//! reset reason string
const char *espResetReasonStr(const esp_reset_reason_t reason);

//! wifi auth mode string
const char *wifiAuthModeStr(const wifi_auth_mode_t mode);

//! wifi cipher type string
const char *wifiCipherTypeStr(const wifi_cipher_type_t type);

//@}


/* ***** Allencheibs ***************************************************************************** */

/*!
    \name Allencheibs
    @{
*/

//! get system ID
/*!
    \returns a string of length 6 based on the factory programmed base mac address
*/
const char *getSystemId(void);

//! get system name, e.g. for making a hostname
/*!
    \param[out] name  system name
    \param[in]  size  size of \c name buffer
    \returns the length of the generated name
*/
int getSystemName(char *name, const int size);

//@}


/* ***** JSMN helpers **************************************************************************** */

/*!
    \name JSMN helpers
    @{
*/


//@}


#endif // __STUFF_H__
//@}
