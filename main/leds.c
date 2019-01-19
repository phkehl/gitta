/*!
    \brief GITTA Tschenggins Lämpli: LEDS WS2801 and SK9822 LED driver (see \ref FF_LEDS)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    - Credits: see source code

*/

#include "stdinc.h"

#include <driver/spi_master.h>

#include "stuff.h"
#include "debug.h"
#include "mon.h"
#include "config.h"
#include "hsv2rgb.h"
#include "leds.h"

#define LEDS_FPS 100

#if (CONFIG_FF_LEDS_NUM < CONFIG_FF_JENKINS_MAX_CH)
#  error CONFIG_FF_LEDS_NUM < CONFIG_FF_JENKINS_MAX_CH!
#endif


/* *********************************************************************************************** */

// LED frame buffer
static uint8_t sLedsData[CONFIG_FF_LEDS_NUM][3];

static void sLedsClear(void)
{
    memset(&sLedsData, 0, sizeof(sLedsData));
}

enum { _R_ = 0, _G_ = 1, _B_ = 2 };

static void sLedsSetRGB(const uint16_t ix, const uint8_t R, const uint8_t G, const uint8_t B)
{
    if (ix < CONFIG_FF_LEDS_NUM)
    {
        switch (configGetOrder())
        {
            case CONFIG_ORDER_RGB: sLedsData[ix][0] = R; sLedsData[ix][1] = G; sLedsData[ix][2] = B; break;
            case CONFIG_ORDER_RBG: sLedsData[ix][0] = R; sLedsData[ix][1] = B; sLedsData[ix][2] = G; break;
            case CONFIG_ORDER_GRB: sLedsData[ix][0] = G; sLedsData[ix][1] = R; sLedsData[ix][2] = B; break;
            case CONFIG_ORDER_GBR: sLedsData[ix][0] = G; sLedsData[ix][1] = B; sLedsData[ix][2] = R; break;
            case CONFIG_ORDER_BRG: sLedsData[ix][0] = B; sLedsData[ix][1] = R; sLedsData[ix][2] = G; break;
            case CONFIG_ORDER_BGR: sLedsData[ix][0] = B; sLedsData[ix][1] = G; sLedsData[ix][2] = R; break;
            case CONFIG_ORDER_UNKNOWN:
            {
                const uint8_t RGB = ((uint16_t)R + (uint16_t)G + (uint16_t)B) / 3;
                sLedsData[ix][0] = RGB; sLedsData[ix][1] = RGB; sLedsData[ix][2] = RGB;
                break;
            }
        }
    }
}

static void sLedsSetHSV(const uint16_t ix, const uint8_t H, const uint8_t S, const uint8_t V)
{
    if (ix < CONFIG_FF_LEDS_NUM)
    {
        uint8_t R, G, B;
        hsv2rgb(H, S, V, &R, &G, &B);
        sLedsSetRGB(ix, R, G, B);
    }
}

static int sLedsRenderWS2801(uint8_t *outBuf, const int bufSize)
{
    memset(outBuf, 0, bufSize);
    const int outSize = MIN(bufSize, (int)sizeof(sLedsData));
    uint32_t brightness = 0;
    switch (configGetBright())
    {
        case CONFIG_BRIGHT_FULL:   brightness =   0; break;
        case CONFIG_BRIGHT_HIGH:   brightness = 200; break;
        case CONFIG_BRIGHT_MEDIUM: brightness = 100; break;
        case CONFIG_BRIGHT_UNKNOWN:
        case CONFIG_BRIGHT_LOW:    brightness =  50; break;
    }
    if (brightness == 0)
    {
        memcpy(outBuf, sLedsData, outSize);
    }
    else
    {
        const uint32_t thrs = 256 / brightness;
        const uint8_t *inBuf = (const uint8_t *)sLedsData;
        for (int ix = 0; ix < bufSize; ix++)
        {
            const uint32_t in = inBuf[ix];
            if (in != 0) { outBuf[ix] = (in <= thrs) ? 1 : ((in * brightness) >> 8); } else { outBuf[ix] = in; }
        }
    }
    return outSize;
}

#define LEDS_SK9822_END_BYTES ( CONFIG_FF_LEDS_NUM / 2 / 8 + 1 )

static int sLedsRenderSK9822(uint8_t *outBuf, const int bufSize)
{
    memset(outBuf, 0, bufSize);

    uint8_t brightness = 0;
    switch (configGetBright())
    {
        case CONFIG_BRIGHT_FULL:   brightness = 31; break;
        case CONFIG_BRIGHT_HIGH:   brightness = 20; break;
        case CONFIG_BRIGHT_MEDIUM: brightness = 10; break;
        case CONFIG_BRIGHT_UNKNOWN:
        case CONFIG_BRIGHT_LOW:    brightness =  5; break;
    }

    // Tim (https://cpldcpu.wordpress.com/2016/12/13/sk9822-a-clone-of-the-apa102/) says:
    // «A protocol that is compatible to both the SK9822 and the APA102 consists of the following:
    //  1. A start frame of 32 zero bits (<0x00> <0x00> <0x00> <0x00> )
    //  2. A 32 bit LED frame for each LED in the string (<0xE0+brightness> <blue> <green> <red>)
    //  3. A SK9822 reset frame of 32 zero bits (<0x00> <0x00> <0x00> <0x00> ).
    //  4. An end frame consisting of at least (n/2) bits of 0, where n is the number of LEDs in the string.»

    int outIx = 0;

    // 1. start frame
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;

    // 2. LEDs data
    for (int ix = 0; (ix < CONFIG_FF_LEDS_NUM) && (outIx < (bufSize - 4 - LEDS_SK9822_END_BYTES )); ix++)
    {
        outBuf[outIx++] = 0xe0 | (brightness & 0x1f); // global brightness
        outBuf[outIx++] = sLedsData[ix][0];
        outBuf[outIx++] = sLedsData[ix][1];
        outBuf[outIx++] = sLedsData[ix][2];
    }

    // 3. reset frame
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;
    outBuf[outIx++] = 0x00;

    // 4. end frame
    int n = LEDS_SK9822_END_BYTES;
    while (n-- > 0)
    {
        outBuf[outIx++] = 0x00;
    }

    return outIx;
}


/* *********************************************************************************************** */

#define LEDS_WS2801_BUFSIZE   (CONFIG_FF_LEDS_NUM * 3)
#define LEDS_SK9822_BUFSIZE   ( 4 + (CONFIG_FF_LEDS_NUM * 4) + 4 + LEDS_SK9822_END_BYTES )

// copy of working frame buffer for transferring to SPI
DRAM_ATTR WORD_ALIGNED_ATTR static uint8_t sLedsSpiBuf[ MAX(LEDS_WS2801_BUFSIZE, LEDS_SK9822_BUFSIZE) ];

// assert that the SPI speed will be fast enough to send the data in time
#if (CONFIG_FF_LEDS_SPI_SPEED / (8 + 2)) < (LEDS_FPS * MAX(LEDS_WS2801_BUFSIZE, LEDS_SK9822_BUFSIZE))
#  error FF_LEDS_SPI_SPEED too slow for CONFIG_FF_LEDS_NUM LEDs!
#endif

static spi_device_handle_t spLedsSpiDevHandle;

// update LEDs (send data to SPI)
static void sLedsFlush(const CONFIG_DRIVER_t driver)
{
     // copy framebuffer
    int nBytesToSend = 0;
    switch (driver)
    {
        case CONFIG_DRIVER_UNKNOWN:
        case CONFIG_DRIVER_NONE:
            break;
        case CONFIG_DRIVER_WS2801:
            nBytesToSend = sLedsRenderWS2801((uint8_t *)sLedsSpiBuf, sizeof(sLedsSpiBuf));
            break;
        case CONFIG_DRIVER_SK9822:
            nBytesToSend = sLedsRenderSK9822((uint8_t *)sLedsSpiBuf, sizeof(sLedsSpiBuf));
            break;
    }

    // initiate transfer and wait until all data has been transmitted
    if (nBytesToSend > 0)
    {
        spi_transaction_t spiTrans;
        memset(&spiTrans, 0, sizeof(spiTrans));
        spiTrans.length    = nBytesToSend * 8; // in bits
        spiTrans.tx_buffer = sLedsSpiBuf;
        esp_err_t ret = spi_device_transmit(spLedsSpiDevHandle, &spiTrans);
        if (ret != ESP_OK)
        {
            WARNING("leds: spi %s", esp_err_to_name(ret));
        }
    }
}


/* *********************************************************************************************** */

typedef struct LEDS_STATE_s
{
    LEDS_PARAM_t param;
    bool         inited;
    uint8_t      val;
    int          count;

} LEDS_STATE_t;

static LEDS_STATE_t sLedsStates[CONFIG_FF_LEDS_NUM];

static SemaphoreHandle_t sLedsStateMutex;

#define LEDS_PULSE_MIN_VAL 10

void ledsSetState(const uint16_t ledIx, const LEDS_PARAM_t *pkParam)
{
    if (ledIx < CONFIG_FF_LEDS_NUM)
    {
        xSemaphoreTake(sLedsStateMutex, portMAX_DELAY);
        //if (memcmp(&sLedsStates[ledIx].param, pkParam, sizeof(*pkParam)) != 0)
        {
            memset(&sLedsStates[ledIx], 0, sizeof(*sLedsStates));
            sLedsStates[ledIx].param = *pkParam;
        }
        xSemaphoreGive(sLedsStateMutex);
    }
}

static const int sLedsPulseAmpl[] =
{
#if (LEDS_FPS == 100)
    // floor(sin(0:pi/2/100:pi).*100)
    0, 1, 3, 4, 6, 7, 9, 10, 12, 14, 15, 17, 18, 20, 21, 23, 24, 26, 27, 29, 30, 32, 33, 35, 36, 38, 39,
    41, 42, 43, 45, 46, 48, 49, 50, 52, 53, 54, 56, 57, 58, 60, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71,
    72, 73, 75, 76, 77, 78, 79, 79, 80, 81, 82, 83, 84, 85, 86, 86, 87, 88, 89, 89, 90, 91, 91, 92, 92,
    93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 97, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 99, 100, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 97, 97, 97, 96, 96, 96, 95, 95, 94, 94, 93, 92, 92, 91,
    91, 90, 89, 89, 88, 87, 86, 86, 85, 84, 83, 82, 81, 80, 79, 79, 78, 77, 76, 75, 73, 72, 71, 70, 69,
    68, 67, 66, 64, 63, 62, 61, 60, 58, 57, 56, 54, 53, 52, 50, 49, 48, 46, 45, 43, 42, 41, 39, 38, 36,
    35, 33, 32, 30, 29, 27, 26, 24, 23, 21, 20, 18, 17, 15, 14, 12, 10, 9, 7, 6, 4, 3, 1, 0,
#else
#  error missing LUT for LEDS_FPS value
#endif
};

static void sLedsRenderFx(LEDS_STATE_t *pState, uint8_t *pHue, uint8_t *pSat, uint8_t *pVal)
{
    if (!pState->inited)
    {
        switch (pState->param.fx)
        {
            case LEDS_FX_STILL:
                break;
            case LEDS_FX_PULSE:
                pState->count = pState->param.arg;
                break;
            case LEDS_FX_FLICKER:
                pState->count = pState->param.arg;
                break;
            case LEDS_FX_BLINK:
                pState->count = -pState->param.arg;
                break;
        }
        pState->inited = true;
    }

    uint8_t hue = 0, sat = 0, val = 0;

    switch (pState->param.fx)
    {
        case LEDS_FX_STILL:
        {
            hue = pState->param.hue;
            sat = pState->param.sat;
            val = pState->param.val;
            break;
        }
        case LEDS_FX_PULSE:
        {
            const uint8_t minVal = MAX(10, pState->param.val / 10);
            pState->val = minVal + (( (pState->param.val - minVal) * sLedsPulseAmpl[pState->count] ) / 100);
            pState->count++;
            pState->count %= NUMOF(sLedsPulseAmpl);
            hue = pState->param.hue;
            sat = pState->param.sat;
            val = pState->val;
            break;
        }
        case LEDS_FX_FLICKER:
        {
            // probabilites by "Eric", commented on
            // https://cpldcpu.wordpress.com/2016/01/05/reverse-engineering-a-real-candle/#comment-1809
            // Probability  Random LED Brightness                               value 0..255
            //  50%          77% -  80% (its barely noticeable)                 196..204
            //  30%          80% - 100% (very noticeable, sim. air flicker)     204..255
            //   5%          50% -  80% (very noticeable, blown out flame)      128..204
            //   5%          40% -  50% (very noticeable, blown out flame)      102..128
            //  10%          30% -  40% (very noticeable, blown out flame)       77..102
            // Probability  Random Time (Duration)
            //  90%          20 ms
            //   3%          20 - 30 ms
            //   3%          10 - 20 ms
            //   4%           0 - 10 ms
            if (pState->count == 0)
            {
                const int pBright = rand() % 100;
                if      (pBright < 50)  { pState->val = 196 + (rand() % (204 - 196)); }
                else if (pBright < 80)  { pState->val = 204 + (rand() % (255 - 204)); }
                else if (pBright < 85)  { pState->val = 128 + (rand() % (204 - 128)); }
                else if (pBright < 90)  { pState->val = 102 + (rand() % (128 - 102)); }
                else                    { pState->val =  77 + (rand() % (102 -  77)); }

                const int pTime = rand() % 100;
                if      (pTime < 90) { pState->count =  20                         / (1000 / LEDS_FPS / 2); }
                else if (pTime < 93) { pState->count = (20 + (rand() % (30 - 20))) / (1000 / LEDS_FPS / 2); }
                else if (pTime < 96) { pState->count = (10 + (rand() % (20 - 10))) / (1000 / LEDS_FPS / 2); }
                else                 { pState->count = (      rand() %  10       ) / (1000 / LEDS_FPS / 2); }
            }
            else
            {
                pState->count--;
            }
            hue = pState->param.hue;
            sat = pState->param.sat;
            val = pState->val / (pState->param.val != 0 ? (256 / pState->param.val) : 1);
            break;
        }
        case LEDS_FX_BLINK:
        {
            if (pState->param.arg > 0)
            {
                pState->count++;
                if (pState->count >= pState->param.arg)
                {
                    pState->param.arg = -pState->param.arg;
                }
                hue = pState->param.hue;
                sat = pState->param.sat;
                val = pState->param.val;
            }
            else if (pState->param.arg < 0)
            {
                pState->count--;
                if (pState->count <= pState->param.arg)
                {
                    pState->param.arg = -pState->param.arg;
                }
                hue = 0;
                sat = 0;
                val = 0;
            }
            break;
        }
    }

    *pHue = hue;
    *pSat = sat;
    *pVal = val;
}

static void sLedsTask(void *pArg)
{
    UNUSED(pArg);

    static CONFIG_DRIVER_t sConfigDriverLast = CONFIG_DRIVER_UNKNOWN;
    static CONFIG_ORDER_t  sConfigOrderLast  = CONFIG_ORDER_UNKNOWN;
    static CONFIG_BRIGHT_t sConfigBrightLast = CONFIG_BRIGHT_UNKNOWN;

    while (true)
    {
        const CONFIG_DRIVER_t configDriver = configGetDriver();
        const CONFIG_ORDER_t  configOrder  = configGetOrder();
        const CONFIG_BRIGHT_t configBright = configGetBright();

        // handle config changes
        bool doDemo = false;
        if (sConfigDriverLast != configDriver)
        {
            DEBUG("leds: driver change");
            sLedsClear();
            sLedsFlush(sConfigDriverLast);
            sConfigDriverLast = configDriver;
            doDemo = true;
        }
        if (sConfigOrderLast != configOrder)
        {
            DEBUG("leds: order change");
            sConfigOrderLast = configOrder;
            doDemo = true;
        }
        if (sConfigBrightLast != configBright)
        {
            DEBUG("leds: bright change");
            sConfigBrightLast = configBright;
            //doDemo = true;
        }

        // cannot do much if we don't know the driver
        if (configGetDriver() == CONFIG_DRIVER_UNKNOWN)
        {
            osSleep(100);
            continue;
        }

        // demo?
        if (doDemo)
        {
            DEBUG("leds: demo");
            for (uint16_t ix = 0; ix < CONFIG_FF_LEDS_NUM; ix += 3)
            {
                sLedsSetRGB(ix + 0, 255, 0, 0);
                sLedsSetRGB(ix + 1, 0, 255, 0);
                sLedsSetRGB(ix + 2, 0, 0, 255);
            }
            for (uint16_t n = 0; n < 20; n++)
            {
                sLedsFlush(configDriver);
                osSleep(100);
            }
        }

        // render next frame..
        static uint32_t sTick;
        sLedsClear();
        for (uint16_t ix = 0; ix < NUMOF(sLedsStates); ix++)
        {
            uint8_t h = 0, s = 0, v = 0;
            xSemaphoreTake(sLedsStateMutex, portMAX_DELAY);
            sLedsRenderFx(&sLedsStates[ix], &h, &s, &v);
            xSemaphoreGive(sLedsStateMutex);
            sLedsSetHSV(ix, h, s, v);
        }
        vTaskDelayUntil(&sTick, MS2TICKS(1000 / LEDS_FPS));
        sLedsFlush(configDriver);
    }
}

/* *********************************************************************************************** */

void ledsInit(void)
{
    INFO("leds: init (%ux3=%u / %u, %u / %u*4=%u, %ukHz, MOSI %d, SCK %d)",
        CONFIG_FF_LEDS_NUM, sizeof(sLedsData),
        LEDS_WS2801_BUFSIZE, LEDS_SK9822_BUFSIZE,
        NUMOF(sLedsSpiBuf), sizeof(sLedsSpiBuf), CONFIG_FF_LEDS_SPI_SPEED / 1000,
        CONFIG_FF_LEDS_MOSI_GPIO, CONFIG_FF_LEDS_SCK_GPIO);

    memset(&sLedsStates, 0, sizeof(sLedsStates));

    // setup SPI hardware
    spi_bus_config_t spiBusCfg =
    {
        .miso_io_num   = -1,
        //.mosi_io_num   = 19, // NINA 21 (on MKR1010: --> SAMD21 PA14 (28))
        //.sclk_io_num   = 18, // NINA 29 (on MKR1010: --> SAMD21 PA15 (29))
        .mosi_io_num   = CONFIG_FF_LEDS_MOSI_GPIO,
        .sclk_io_num   = CONFIG_FF_LEDS_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = sizeof(sLedsSpiBuf),
    };
    ASSERT_ESP_OK( spi_bus_initialize(HSPI_HOST, &spiBusCfg, /* DMA channel: */ 1) );
    spi_device_interface_config_t spiDevIfCfg =
    {
        .clock_speed_hz = CONFIG_FF_LEDS_SPI_SPEED,
        .mode           = 0,
        .spics_io_num   = -1,
        .queue_size     = 1,
    };
    ASSERT_ESP_OK( spi_bus_add_device(HSPI_HOST, &spiDevIfCfg, &spLedsSpiDevHandle) );
    ASSERT_ESP_OK( spLedsSpiDevHandle == NULL ? ESP_FAIL : ESP_OK );

    // send "clear all LEDs" for all known drivers, this should clear all LEDs in all(most all) setups
    sLedsClear();
    sLedsFlush(CONFIG_DRIVER_SK9822);
    osSleep(100);
    sLedsFlush(CONFIG_DRIVER_WS2801);
    osSleep(100);
}

void ledsStart(void)
{
    INFO("leds: start");

    static StaticSemaphore_t sMutex;
    sLedsStateMutex = xSemaphoreCreateMutexStatic(&sMutex);

    static StackType_t sLedsTaskStack[4096 / sizeof(StackType_t)];
    static StaticTask_t sLedsTaskTCB;
    xTaskCreateStaticPinnedToCore(sLedsTask, CONFIG_FF_NAME"_leds", NUMOF(sLedsTaskStack),
        NULL, CONFIG_FF_LEDS_PRIO, sLedsTaskStack, &sLedsTaskTCB, 1);
}


/* *********************************************************************************************** */
// eof
