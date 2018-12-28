/*
    \file
    \brief GITTA Tschenggins Lämpli: main (see \ref FF_MAIN)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli

    \defgroup GITTA GITTA
    @{
    @}
*/

#include "stdinc.h"
#include "stuff.h"
#include "debug.h"
#include "mon.h"
#include "console.h"
#include "env.h"
#include "wifi.h"
#include "status.h"
#include "tone.h"
#include "leds.h"
#include "config.h"
#include "jenkins.h"
#include "experiments.h"
#include "version_gen.h"

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
void app_main()
{
    // initialise things
    debugInit(); // must be first
    envInit();
    stuffInit();
    statusInit();
    toneInit();
    ledsInit();
    configInit();
    jenkinsInit();

    debugSetLevel( envGet("debug") );

    // use factory programmed mac addresses
    uint8_t macBase[6];
    esp_efuse_mac_get_default(macBase);
    esp_base_mac_addr_set(macBase);

    // say hello
    char name[32];
    getSystemName(name, sizeof(name));
    INFO("------------------------------------------------------------------------------------------");
    INFO("GITTA Tschenggins Lämpli :-) - "FF_VERSION" - %s", name);
    INFO("Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>");
    INFO("https://oinkzwurgl.org/projaeggd/tschenggins-laempli/");
    INFO("Parts copyright by others. See source code.");
    INFO("------------------------------------------------------------------------------------------");

    // print some system information
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    DEBUG("%s, %d cores, rev %d%s%s%s, %dMB %s flash, cache %s",
        chipInfo.model == CHIP_ESP32 ? "ESP32" : "ESP??",
        chipInfo.cores, chipInfo.revision,
        (chipInfo.features & CHIP_FEATURE_WIFI_BGN) ? ", WiFi BGN" : "",
        (chipInfo.features & CHIP_FEATURE_BT) ? ", BT" : "",
        (chipInfo.features & CHIP_FEATURE_BLE) ? ", BLE" : "",
        spi_flash_get_chip_size() / (1024 * 1024),
        (chipInfo.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external",
        spi_flash_cache_enabled() ? "enabled" : "disabled");
    DEBUG("IDF %s", esp_get_idf_version());
    DEBUG("reset: %s", espResetReasonStr(esp_reset_reason()));
    uint8_t macSta[6], macAp[6], macBt[6], macEth[6];
    esp_read_mac(macSta, ESP_MAC_WIFI_STA);
    esp_read_mac(macAp, ESP_MAC_WIFI_SOFTAP);
    esp_read_mac(macBt, ESP_MAC_BT);
    esp_read_mac(macEth, ESP_MAC_ETH);
    DEBUG("mac: sta="MACSTR" ap="MACSTR" bt="MACSTR" eth="MACSTR,
        MAC2STR(macSta), MAC2STR(macAp), MAC2STR(macBt), MAC2STR(macEth));

    // start things
    monStart();

    // experiments
    //exToggleSpivPins();
    //exSendSpiv();
    //exStatusLedTest();
    //exTone();
    //exStatusNoise();
    //exScanWifi();

    // start more things
    consoleStart();
    wifiStart();
    ledsStart();
    jenkinsStart();
}


// eof
