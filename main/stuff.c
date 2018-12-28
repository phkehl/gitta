/*
    \file
    \brief GITTA Tschenggins Lämpli: stuff (see \ref FF_STUFF)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"
#include "debug.h"
#include "stuff.h"

static uint32_t sOsTimePosix;
static uint32_t sOsTimeOs;

void osSetPosixTime(const uint32_t timestamp)
{
    sOsTimeOs = osTime();
    sOsTimePosix = timestamp;
    //DEBUG("osSetPosixTime() %u %u", sOsTimePosix, sOsTimeOs);
}

uint32_t osGetPosixTime(void)
{
    const uint32_t now = osTime();
    //DEBUG("osGetPosixTime() %u %u-%u=%u", sOsTimePosix, now, sOsTimeOs, now - sOsTimeOs);
    return sOsTimePosix + ((now - sOsTimeOs) / 1000);
}

const char *espResetReasonStr(const esp_reset_reason_t reason)
{
    switch (reason)
    {
        case ESP_RST_UNKNOWN:   return "UNKNOWN";
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXT";
        case ESP_RST_SW:        return "SW";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
    }
    return "???";
}

const char *wifiAuthModeStr(const wifi_auth_mode_t mode)
{
    switch (mode)
    {
        case WIFI_AUTH_OPEN:            return "OPEN";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPR";
        default:
            break;
    }
    return "???";
}

const char *wifiCipherTypeStr(const wifi_cipher_type_t type)
{
    switch (type)
    {
        case WIFI_CIPHER_TYPE_NONE:      return "NONE";
        case WIFI_CIPHER_TYPE_WEP40:     return "WEP40";
        case WIFI_CIPHER_TYPE_WEP104:    return "WEP104";
        case WIFI_CIPHER_TYPE_TKIP:      return "TKIP";
        case WIFI_CIPHER_TYPE_CCMP:      return "CCMP";
        case WIFI_CIPHER_TYPE_TKIP_CCMP: return "TKIP_CCMP";
        case WIFI_CIPHER_TYPE_UNKNOWN:   return "UNKNOWN";
    }
    return "???";
}

int getSystemName(char *name, const int size)
{
    name[0] = '\0';
    strncpy(name, CONFIG_FF_NAME, size);
    name[size - 1] = '\0';
    if (size >= 12)
    {
        const int len = strlen(name);
        sprintf(&name[ len > (size - 8) ? (size - 8) : len ], "-%s", getSystemId());
    }
    const uint8_t len = strlen(name);
    return len;
}

const char *getSystemId(void)
{
    static char name[7];
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac); // factory programmed base mac address
    snprintf(name, sizeof(name), "%02x%02x%02x", mac[3], mac[4], mac[5]);
    return name;
}

void stuffInit(void)
{
    INFO("stuff: init");
}

// eof
