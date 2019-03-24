/*!
    \file
    \brief GITTA Tschenggins Lämpli: system status (see \ref FF_STATUS)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"

#include "stuff.h"
#include "debug.h"
#include "tone.h"
#include "cfg.h"
#include "status.h"

static uint8_t sStatusLedPeriod;
static uint8_t sStatusLedNum;
static uint8_t sStatusEffectDur;

static void sStatusLedTimerFunc(TimerHandle_t timer)
{
    UNUSED(timer);
    static uint32_t tick = 0;
    if (sStatusLedPeriod && sStatusLedNum)
    {
        const uint8_t phase = tick % sStatusLedPeriod;
        if ( phase < (2 * sStatusLedNum) )
        {
            gpio_set_level(CONFIG_FF_STATUS_LED_GPIO, (phase % 2) == 0 ? true : false);
        }
    }
    tick++;

    if (sStatusEffectDur > 0)
    {
        sStatusEffectDur--;
        if (sStatusEffectDur == 0)
        {
            gpio_set_level(CONFIG_FF_STATUS_EFFECT_GPIO, false);
        }
    }
}

void statusLed(const STATUS_LED_t status)
{
    gpio_set_level(CONFIG_FF_STATUS_LED_GPIO, false);
    switch (status)
    {
        case STATUS_LED_NONE:
            DEBUG("status: none");
            break;
        case STATUS_LED_HEARTBEAT:
            DEBUG("status: heartbeat");
            sStatusLedPeriod = 20;
            sStatusLedNum    = 2;
            break;
        case STATUS_LED_OFFLINE:
            DEBUG("status: offline");
            sStatusLedPeriod = 20;
            sStatusLedNum    = 1;
            break;
        case STATUS_LED_FAIL:
            DEBUG("status: fail");
            sStatusLedPeriod = 20;
            sStatusLedNum    = 5;
            break;
        case STATUS_LED_UPDATE:
            DEBUG("status: update");
            sStatusLedPeriod = 2;
            sStatusLedNum    = 1;
            break;
    }
}

void statusNoise(const STATUS_NOISE_t noise)
{
    DEBUG("status: noise %d", noise);

    if (toneIsPlaying() || (cfgGetNoise() == CFG_NOISE_NONE) )
    {
        return;
    }

    switch (noise)
    {
        case STATUS_NOISE_ABORT:
        {
            static const int16_t skNoiseAbort[] =
            {
                TONE(A5, 30), TONE(PAUSE, 20), TONE(G5, 60), TONE_END
            };
            toneMelody(skNoiseAbort);
            break;
        }
        case STATUS_NOISE_FAIL:
        {
            static const int16_t skNoiseFail[] =
            {
                TONE(A5, 30), TONE(PAUSE, 20), TONE(G5, 60), TONE(PAUSE, 20), TONE(F5, 100), TONE_END
            };
            toneMelody(skNoiseFail);
            break;
        }
        case STATUS_NOISE_ONLINE:
        {
            static const int16_t skNoiseOnline[] =
            {
                TONE(D6, 30), TONE(PAUSE, 20), TONE(E6, 60), TONE_END
            };
            toneMelody(skNoiseOnline);
            break;
        }
        case STATUS_NOISE_OTHER:
        {
            static const int16_t skNoiseOther[] =
            {
                TONE(C6, 30), TONE_END
            };
            toneMelody(skNoiseOther);
            break;
        }
        case STATUS_NOISE_ERROR:
        {
            static const int16_t skNoiseError[] =
            {
                TONE(C4, 200), TONE(PAUSE, 50), TONE(C4, 200), TONE_END
            };
            toneMelody(skNoiseError);
            break;
        }
        case STATUS_NOISE_TICK:
        {
            static const int16_t skNoiseError[] =
            {
                TONE(C8, 40), TONE(PAUSE, 30), TONE(C7, 40), TONE_END
            };
            toneMelody(skNoiseError);
            break;
        }
    }
}

void statusMelody(const char *name)
{
    DEBUG("status: melody %s", name);

    if (cfgGetNoise() < CFG_NOISE_MORE)
    {
        return;
    }
    toneStop();
    toneBuiltinMelody(name);
}

void statusEffect(void)
{
    DEBUG("status: effect");
    gpio_set_level(CONFIG_FF_STATUS_EFFECT_GPIO, true);
    sStatusEffectDur = CONFIG_FF_STATUS_EFFECT_DURATION;
}

void statusInit(void)
{
    INFO("status: init (GPIO: LED %d, fx %d)", CONFIG_FF_STATUS_LED_GPIO, CONFIG_FF_STATUS_EFFECT_GPIO);

    gpio_pad_select_gpio(CONFIG_FF_STATUS_LED_GPIO);
    gpio_set_direction(  CONFIG_FF_STATUS_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(      CONFIG_FF_STATUS_LED_GPIO, false);

    gpio_pad_select_gpio(CONFIG_FF_STATUS_EFFECT_GPIO);
    gpio_set_direction(  CONFIG_FF_STATUS_EFFECT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(      CONFIG_FF_STATUS_EFFECT_GPIO, false);

    // setup LED timer
    static StaticTimer_t sTimer;
    TimerHandle_t timer = xTimerCreateStatic("status_led", MS2TICKS(100), true, NULL, sStatusLedTimerFunc, &sTimer);
    xTimerStart(timer, 1000);

    statusLed(STATUS_LED_NONE);
}

// eof
