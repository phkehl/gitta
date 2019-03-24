/*!
    \file
    \brief GITTA Tschenggins Lämpli: tones and melodies (see \ref FF_TONE)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \addtogroup FF_TONE

    @{
*/

#include "stdinc.h"

#include <driver/timer.h>

#include "stuff.h"
#include "debug.h"
#include "mon.h"
#include "tone.h"

/* *********************************************************************************************** */

#define TONE_TIMER_GROUP       TIMER_GROUP_0
#define TONE_TIMER_IX          TIMER_0
#define TONE_GPIO              CONFIG_FF_TONE_GPIO
#define TONE_TIMER_GROUP_REG   TIMERG0
#define TONE_TIMER_DIV         2

#if (TONE_GPIO < 32)
#  define TONE_GPIO_HIGH() GPIO.out_w1ts = BIT(TONE_GPIO)
#  define TONE_GPIO_LOW()  GPIO.out_w1tc = BIT(TONE_GPIO)
#else
#  define TONE_GPIO_HIGH() GPIO.out1_w1ts.data = BIT(TONE_GPIO - 32)
#  define TONE_GPIO_LOW()  GPIO.out1_w1tc.data = BIT(TONE_GPIO - 32)
#endif

// forward declarations
static void sToneIsr(void *pArg);
static void sToneStop(void);
static void sToneStart(void);

static volatile bool sToneIsPlaying;

void toneStart(const uint32_t freq, const uint32_t dur)
{
    sToneStop();

    // set "melody"
    const int16_t pkFreqDur[] = { freq, dur, TONE_END };
    toneMelody(pkFreqDur);
}

#define TONE_MELODY_N 50

static volatile uint32_t svToneMelodyTimervals[TONE_MELODY_N + 1];
static volatile int16_t  svToneMelodyTogglecnts[TONE_MELODY_N + 1];
static volatile int16_t  svToneMelodyIx;

#define PAUSE_FREQ 1000

void toneMelody(const int16_t *pkFreqDur)
{
    UNUSED(pkFreqDur);

    sToneStop();
    uint32_t totalDur = 0;
    uint16_t nNotes = 0;

    //memset(svToneMelodyTimervals, 0, sizeof(svToneMelodyTimervals));
    for (int ix = 0; ix < (int)NUMOF(svToneMelodyTimervals); ix++)
    {
        svToneMelodyTimervals[ix] = 0;
    }
    //memset(svToneMelodyTogglecnts, 0, sizeof(svToneMelodyTogglecnts));
    for (int ix = 0; ix < (int)NUMOF(svToneMelodyTogglecnts); ix++)
    {
        svToneMelodyTogglecnts[ix] = 0;
    }
    svToneMelodyIx = 0;

    for (int16_t ix = 0; ix < TONE_MELODY_N; ix++)
    {
        const int16_t freq = pkFreqDur[ 2 * ix ];
        if ( (freq != TONE_END) && (freq > 0) )
        {
            const int16_t _freq = freq != TONE_PAUSE ? freq : PAUSE_FREQ;
            const int16_t dur = pkFreqDur[ (2 * ix) + 1 ];

            // timer value
            const uint32_t timerval = TIMER_BASE_CLK /* = APB_CLK_FREQ */ / TONE_TIMER_DIV / 2 / _freq;

            // number of times to toggle the PIO
            const int16_t togglecnt = 2 * _freq * dur / 1000;

            svToneMelodyTimervals[ix]  = timerval;
            svToneMelodyTogglecnts[ix] = freq != TONE_PAUSE ? togglecnt : -togglecnt;

            totalDur += dur;
            nNotes++;

            //DEBUG("toneMelody() %2d %4d %4d -> %6u %4d", ix, freq, dur, timerval, togglecnt);
        }
        else
        {
            break;
        }
    }

    //DEBUG("toneMelody() %ums, %u", totalDur, nNotes);

    // configure hw timer
    const timer_config_t timerConfig =
    {
        .alarm_en    = TIMER_ALARM_EN,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = true,
        .divider     = 2,

    };
    timer_init(TONE_TIMER_GROUP, TONE_TIMER_IX, &timerConfig);

    timer_set_counter_value(TIMER_GROUP_0, TONE_TIMER_IX, 0);
    //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].load_high = 0;
    //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].load_low = 0;
    //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].reload = 1;
    timer_set_alarm_value(TONE_TIMER_GROUP, TONE_TIMER_IX, 0xffffffff);

    // enable and unmask interrupt
    timer_enable_intr(TONE_TIMER_GROUP, TONE_TIMER_IX);

    sToneIsPlaying = true;
    sToneStart();
}

/* *********************************************************************************************** */

static volatile int16_t svToneToggleCnt;
static volatile bool svToneSilent;

static void sToneStart(void) // RAM func
{
    gpio_set_level(TONE_GPIO, false);

    const int32_t timerval  = svToneMelodyTimervals[svToneMelodyIx];
    const int16_t togglecnt = svToneMelodyTogglecnts[svToneMelodyIx];
    svToneMelodyIx++;

    if (togglecnt)
    {
        svToneToggleCnt = togglecnt < 0 ? -togglecnt : togglecnt;
        svToneSilent = togglecnt < 0 ? true : false;

        // arm timer (23 bits, 0-8388607) // FIXME: 64 bits on ESP32
        //timer_set_alarm_value(TONE_TIMER_GROUP, TONE_TIMER_IX, timerval);
        //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].alarm_high = (uint32_t) (timerval >> 32);
        //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].alarm_high = 0;
        TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].alarm_low = (uint32_t)timerval;

        // start timer
        //timer_start(TONE_TIMER_GROUP, TONE_TIMER_IX);
        TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].config.enable = 1;
    }
    else
    {
        sToneStop();
    }
}

static void sToneStop(void) // RAM func
{
    // stop timer

    //timer_pause(TONE_TIMER_GROUP, TONE_TIMER_IX);
    TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].config.enable = 0;

    //timer_disable_intr(TONE_TIMER_GROUP, TONE_TIMER_IX);
    TONE_TIMER_GROUP_REG.int_ena.val &= ~BIT(TONE_TIMER_IX);

    //gpio_set_level(TONE_GPIO, false);
    TONE_GPIO_LOW();

    sToneIsPlaying = false;
}

void toneStop(void)
{
    sToneStop();
}

bool toneIsPlaying(void)
{
    return sToneIsPlaying;
}


IRAM_ATTR static void sToneIsr(void *pArg) // RAM func
{
    //monIsrEnter();
    UNUSED(pArg);

    // clear interrupt
    TONE_TIMER_GROUP_REG.int_clr_timers.val = BIT(TONE_TIMER_IX);

    // ??
    //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].update = 1;

    // toggle PIO...
    if (!svToneSilent)
    {
        if (svToneToggleCnt & 0x1)
        {
            //gpio_set_level(TONE_GPIO, true);
            TONE_GPIO_HIGH();
        }
        else
        {
            //gpio_set_level(TONE_GPIO, false);
            TONE_GPIO_LOW();
        }
    }

    svToneToggleCnt--;

    // enable next timer alarm
    TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].config.alarm_en = 1;

    // ...until done
    if (svToneToggleCnt <= 0)
    {
        sToneStart();
    }


    //monIsrLeave();
}

/* *********************************************************************************************** */

void toneBuiltinMelody(const char *name)
{
    const char *rtttl = rtttlBuiltinMelody(name);
    if (rtttl == NULL)
    {
        ERROR("tone: no such melody: %s", name);
        return;
    }
    toneRtttlMelody(rtttl);
}

void toneBuiltinMelodyRandom(void)
{
    const char *rtttl = rtttlBuiltinMelodyRandom();
    toneRtttlMelody(rtttl);
}


void toneRtttlMelody(const char *rtttl)
{
    toneStop();
    const int nMelody = (TONE_MELODY_N * 2) + 1;
    int16_t *pMelody = malloc( nMelody * sizeof(int16_t) );
    if (pMelody)
    {
        rtttlMelody(rtttl, pMelody, nMelody);
        toneMelody(pMelody);
        free(pMelody);
    }
    else
    {
        ERROR("tone: malloc fail");
    }
}

/* *********************************************************************************************** */


void toneInit(void)
{
    INFO("tone: init (%uMHz, GPIO %d)", TIMER_BASE_CLK /* = APB_CLK_FREQ */ / 1000000, CONFIG_FF_TONE_GPIO);

    gpio_pad_select_gpio(TONE_GPIO);
    gpio_set_direction(  TONE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(      TONE_GPIO, false);

    // disable and mask interrupt
    ASSERT_ESP_OK( timer_disable_intr(TONE_TIMER_GROUP, TONE_TIMER_IX) );

    // stop timer
    ASSERT_ESP_OK( timer_pause(TONE_TIMER_GROUP, TONE_TIMER_IX) );

    // attach ISR
    ASSERT_ESP_OK( timer_isr_register(TONE_TIMER_GROUP, TONE_TIMER_IX, sToneIsr, NULL, ESP_INTR_FLAG_IRAM, NULL) );
}


/* *********************************************************************************************** */
//@}
// eof
