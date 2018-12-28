#include "stdinc.h"
#include "stuff.h"
#include "debug.h"
#include "status.h"
#include "experiments.h"

#define PIN_CLK 18 // SPIV_CLK/GPIO18 --> PA15 (29)
#define PIN_DO  19 // SPIV_DO/GPIO19  --> PA14 (28)
void exToggleSpivPins(void)
{
    gpio_pad_select_gpio(PIN_CLK);
    gpio_set_direction(  PIN_CLK, GPIO_MODE_OUTPUT);
    gpio_set_level(      PIN_CLK, false);
    gpio_pad_select_gpio(PIN_DO);
    gpio_set_direction(  PIN_DO,  GPIO_MODE_OUTPUT);
    gpio_set_level(      PIN_DO,  false);
    int ix = 0;
    bool clkState = false;
    bool doState  = false;
    while (true)
    {
        if ( (ix % 2) == 0 )
        {
            clkState = !clkState;
            DEBUG("dummy: clk %s", clkState ? "high" : "low");
            gpio_set_level(PIN_CLK, clkState);
        }
        if ( (ix % 3) == 0 )
        {
            doState = !doState;
            DEBUG("dummy: do %s", doState ? "high" : "low");
            gpio_set_level(PIN_DO, doState);
        }
        ix++;
        osSleep(250);
    }
}

#include <driver/spi_master.h>
//#include "soc/gpio_struct.h"

void exSendSpiv(void)
{
    // warning: comment out ledsInit() to run this successfully

    spi_bus_config_t spiBusCfg =
    {
        .miso_io_num   = -1,
        .mosi_io_num   = 19, // NINA 21 (--> SAMD21 PA14 (28))
        .sclk_io_num   = 18, // NINA 29 (--> SAMD21 PA15 (29))
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 100,
    };
    ESP_ERROR_CHECK( spi_bus_initialize(HSPI_HOST, &spiBusCfg, 1) );

    spi_device_interface_config_t spiDevIfCfg =
    {
        .clock_speed_hz = CONFIG_FF_LEDS_SPI_SPEED,
        .mode           = 0,
        .spics_io_num   = -1,
        .queue_size     = 1,
    };
    spi_device_handle_t pSpiDevHandle;
    ESP_ERROR_CHECK( spi_bus_add_device(HSPI_HOST, &spiDevIfCfg, &pSpiDevHandle) );

    DRAM_ATTR WORD_ALIGNED_ATTR static uint8_t data[] =
    {
#if 0
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
#else
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#endif
    };

    int ix = 0;
    while (true)
    {
        DEBUG("spi send %d", ix);
        if      (ix == 0) { data[0] = 0xff; data[1] = 0x00; data[2] = 0x00; data[3] = 0xff; data[4] = 0x00; data[5] = 0x00; }
        else if (ix == 1) { data[0] = 0x00; data[1] = 0xff; data[2] = 0x00; data[3] = 0x00; data[4] = 0xff; data[5] = 0x00; }
        else if (ix == 2) { data[0] = 0x00; data[1] = 0x00; data[2] = 0xff; data[3] = 0x00; data[4] = 0x00; data[5] = 0xff; }
        ix++;
        ix %= 3;

        spi_transaction_t spiTrans;
        memset(&spiTrans, 0, sizeof(spiTrans));
        spiTrans.length    = NUMOF(data) * 8; // in bits
        spiTrans.tx_buffer = data;
        //esp_err_t ret = spi_device_polling_transmit(pSpiDevHandle, &spiTrans);
        esp_err_t ret = spi_device_transmit(pSpiDevHandle, &spiTrans);
        DEBUG("spi: %s", esp_err_to_name(ret));

        osSleep(1000);
    }
}

void exStatusLedTest(void)
{
    statusLed(STATUS_LED_HEARTBEAT);
    while (true)
    {
        DEBUG("exStatusLedTest()");
        osSleep(1000);
    }
}


#include <driver/timer.h>
#define TONE_TIMER_GROUP       TIMER_GROUP_0
#define TONE_TIMER_IX          TIMER_1 // Note: must not be the same as in tone.c or the timer_isr_register() below will fail
#define TONE_GPIO              CONFIG_FF_TONE_GPIO
#define TONE_TIMER_GROUP_REG   TIMERG0
#if (TONE_GPIO < 32)
#  define TONE_GPIO_HIGH() GPIO.out_w1ts = BIT(TONE_GPIO)
#  define TONE_GPIO_LOW()  GPIO.out_w1tc = BIT(TONE_GPIO)
#else
#  define TONE_GPIO_HIGH() GPIO.out1_w1ts.data = BIT(TONE_GPIO - 32)
#  define TONE_GPIO_LOW()  GPIO.out1_w1tc.data = BIT(TONE_GPIO - 32)
#endif
static volatile bool sToneToggle;
static volatile uint32_t sToneIsrCount;
IRAM_ATTR static void sToneIsr(void *pArg) // RAM func
{
    UNUSED(pArg);

    // clear interrupt
    TONE_TIMER_GROUP_REG.int_clr_timers.val = BIT(TONE_TIMER_IX);

    //TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].update = 1;

    sToneIsrCount++;
    if (sToneToggle)
    {
        TONE_GPIO_HIGH();
    }
    else
    {
        TONE_GPIO_LOW();
    }
    sToneToggle = !sToneToggle;

    TONE_TIMER_GROUP_REG.hw_timer[TONE_TIMER_IX].config.alarm_en = 1;
}
void exTone(void)
{
    DEBUG("exTone(): init (%uMHz, IO%u)", TIMER_BASE_CLK /* = APB_CLK_FREQ */ / 1000000, TONE_GPIO);
    gpio_pad_select_gpio(TONE_GPIO);
    gpio_set_direction(  TONE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(      TONE_GPIO, false);

    ESP_ERROR_CHECK( timer_disable_intr(TONE_TIMER_GROUP, TONE_TIMER_IX) );
    ESP_ERROR_CHECK( timer_pause(TONE_TIMER_GROUP, TONE_TIMER_IX) );
    ESP_ERROR_CHECK( timer_isr_register(TONE_TIMER_GROUP, TONE_TIMER_IX, sToneIsr, NULL, ESP_INTR_FLAG_IRAM, NULL) );

    const timer_config_t timerConfig =
    {
        .alarm_en    = TIMER_ALARM_EN,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = true,
        .divider     = 2,

    };
    ESP_ERROR_CHECK( timer_init(TONE_TIMER_GROUP, TONE_TIMER_IX, &timerConfig) );
    ESP_ERROR_CHECK( timer_set_counter_value(TIMER_GROUP_0, TONE_TIMER_IX, 0) );
    ESP_ERROR_CHECK( timer_enable_intr(TONE_TIMER_GROUP, TONE_TIMER_IX) );

    const uint32_t freq = 400;
    //const uint32_t timerval = (TIMER_BASE_CLK / 1000000 * 500000) * 2  / freq;
    const uint32_t timerval = (TIMER_BASE_CLK / 2 / 2 / freq );
    DEBUG("timerval=%u", timerval); FLUSH();

    ESP_ERROR_CHECK( timer_set_alarm_value(TONE_TIMER_GROUP, TONE_TIMER_IX, timerval) );
    ESP_ERROR_CHECK( timer_start(TONE_TIMER_GROUP, TONE_TIMER_IX) );


    while (true)
    {
        DEBUG("exTone() %u", sToneIsrCount);
        //gpio_set_level(TONE_GPIO, false);
        osSleep(500);
        //gpio_set_level(TONE_GPIO, true);
        osSleep(500);
    }
}

#include "tone.h"

void exStatusNoise(void)
{
    osSleep(2500);
    while (true)
    {
        DEBUG("exTones()");
        //toneStart(RTTTL_NOTE_A4, 300);
        osSleep(1000);
        statusNoise(STATUS_NOISE_ABORT);
        osSleep(1000);
        statusNoise(STATUS_NOISE_FAIL);
        osSleep(1000);
        statusNoise(STATUS_NOISE_ONLINE);
        osSleep(1000);
        statusNoise(STATUS_NOISE_OTHER);
        osSleep(1000);
        statusNoise(STATUS_NOISE_TICK);
        osSleep(1000);
        statusNoise(STATUS_NOISE_ERROR);
        osSleep(1000);
        toneBuiltinMelodyRandom();
        osSleep(5000);
        toneStop();
        osSleep(2500);
    }
}

static void sWifiScan(void)
{
    printf("wifi: start scan\n");
    esp_err_t err;
    esp_wifi_scan_stop();
    wifi_scan_config_t scanConfig = { .show_hidden = true, /*.scan_type = WIFI_SCAN_TYPE_PASSIVE*/ };
    err = esp_wifi_scan_start(&scanConfig, true);
    if ( err != ESP_OK )
    {
        printf("wifi: scan failed: %s\n", esp_err_to_name(err));
        return false;
    }

    uint16_t numAp;
    err = esp_wifi_scan_get_ap_num(&numAp);
    if (err != ESP_OK )
    {
        return false;
    }
    printf("wifi: done scan (%u ap) %u\n", numAp, sizeof(wifi_ap_record_t));
    if (numAp > 50)
    {
        printf("wifi: too man access points\n");
        return false;
    }
    wifi_ap_record_t *pApRecs = malloc(numAp * sizeof(wifi_ap_record_t));
    if (pApRecs == NULL)
    {
        printf("wifi: malloc\n");
        return false;
    }
    err = esp_wifi_scan_get_ap_records(&numAp, pApRecs);
    if (err != ESP_OK)
    {
        free(pApRecs);
        return false;
    }
    for (int ix = 0; ix < numAp; ix++)
    {
        const wifi_ap_record_t *pkAp = &pApRecs[ix];
        char country[4]; // there can be weird shit in pkAp->country.cc
        country[0] = isprint((int)pkAp->country.cc[0]) != 0 ? pkAp->country.cc[0] : '?';
        country[1] = isprint((int)pkAp->country.cc[1]) != 0 ? pkAp->country.cc[1] : '?';
        country[2] = isprint((int)pkAp->country.cc[2]) != 0 ? pkAp->country.cc[2] : '?';
        country[3] = '\0';
        char ssid[33];
        strncpy(ssid, (const char *)pkAp->ssid, 32);
        ssid[32] = '\0';
        printf("ap %02d/%02d "MACSTR" ch %2u%c rssi %3d %-12s %-9s %-9s %c%c%c %c %s"/* %c%c%c */" %s\n",
            ix + 1, numAp,
            MAC2STR(pkAp->bssid),
            pkAp->primary, pkAp->second == WIFI_SECOND_CHAN_ABOVE ? '^' : (pkAp->second == WIFI_SECOND_CHAN_BELOW ? '_' : '-'),
            pkAp->rssi, wifiAuthModeStr(pkAp->authmode), wifiCipherTypeStr(pkAp->pairwise_cipher),
            wifiCipherTypeStr(pkAp->group_cipher),
            (pkAp->phy_11b ? 'B' : '.'), (pkAp->phy_11g ? 'G' : '.'), (pkAp->phy_11n ? 'N' : '.'),
            (pkAp->wps ? 'W' : '.'), country, /*pkAp->country.cc[0], pkAp->country.cc[1], pkAp->country.cc[2],*/
            (pkAp->ssid[0] == 0 ? "<hidden>" : ssid));
    }
    free(pApRecs);
}
void exScanWifi(void)
{
    const wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&wifiCfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    //ESP_ERROR_CHECK( esp_wifi_set_auto_connect(false) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    sWifiScan();
    while (true)
    {
        DEBUG("exScanWifi()");
        osSleep(1000);
    }

}
// eof
