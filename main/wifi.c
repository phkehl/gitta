/*
    \file
    \brief GITTA Tschenggins Lämpli: wifi and network things (see \ref FF_WIFI)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"

#include <esp_wifi.h>
#include <tcpip_adapter.h>
#include <esp_event_loop.h>
#include <esp_http_client.h>

#include "stuff.h"
#include "debug.h"
#include "mon.h"
#include "env.h"
#include "status.h"
#include "backend.h"
#include "version_gen.h"
#include "wifi.h"

#if (CONFIG_FF_WIFI_RECONNECT_INTERVAL_SLOW <= CONFIG_FF_WIFI_RECONNECT_INTERVAL)
#  error CONFIG_FF_WIFI_RECONNECT_INTERVAL_SLOW <= CONFIG_FF_WIFI_RECONNECT_INTERVAL!
#endif

/* *********************************************************************************************** */

// query parameters for the backend
#define BACKEND_QUERY "cmd=realtime;ascii=1;client=%s;name=%s;stassid=%s;staip="IPSTR";version="FF_VERSION";maxch="STRINGIFY(CONFIG_FF_JENKINS_MAX_CH)
#define BACKEND_ARGS(client, name, stassid, staip) client, name, stassid, staip

typedef struct WIFI_DATA_s
{
    bool valid;
    char ssid[32];
    char backend[CONFIG_FF_BACKEND_URL_MAXLEN];
    char name[32];
    char client[10];
    char params[sizeof(BACKEND_QUERY) + 200];
    esp_http_client_handle_t pHttpClient;
    bool helloSeen;
    BACKEND_STATUS_t backendStatus;

} WIFI_DATA_t;

static esp_err_t sWifiHttpEventHandler(esp_http_client_event_t *pEvent)
{
    switch (pEvent->event_id)
    {
        case HTTP_EVENT_ERROR:
            WARNING("wifi: HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            INFO("wifi: http connected");
            break;
        case HTTP_EVENT_HEADER_SENT:
            DEBUG("wifi: http headers sent");
            break;
        case HTTP_EVENT_ON_HEADER: // noisy!
            //DEBUG("wifi: HTTP_EVENT_ON_HEADER [%s] -> [%s]",
            //    pEvent->header_key, pEvent->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
        {
            // we receive (expect) text, nul terminate it
            char *pData = (char *)pEvent->data;
            int dataLen = pEvent->data_len;
            pData[dataLen] = '\0';
            //DEBUG("wifi: data(%d)=[%s]", dataLen, pData);

            // the first chunk of data must contain the "hello"
            WIFI_DATA_t *pWifiData = (WIFI_DATA_t *)pEvent->user_data;
            if (!pWifiData->helloSeen)
            {
                char *pHello = strstr(pData, "hello ");
                char *pEol = strstr(pData, "\r\n");
                if ( (pHello == NULL) || (pEol == NULL) )
                {
                    WARNING("wifi: no 'hello' in response");
                    pData = NULL;
                    // FIXME: we should be able to "return ESP_FAIL" or something, but
                    // esp_http_client doesn't care about our return value, so do this instead:
                    esp_http_client_close(pEvent->client);
                }
                else
                {
                    *pEol = '\0';
                    DEBUG("wifi: have hello: [%s]", pHello);
                    statusNoise(STATUS_NOISE_ONLINE);
                    statusLed(STATUS_LED_HEARTBEAT);
                    pWifiData->helloSeen = true;

                    // remaining data
                    pData = pEol + 2;
                    dataLen = strlen(pData);
                    if (dataLen <= 0)
                    {
                        pData = NULL;
                        dataLen = 0;
                    }
                }
            }

            // process data
            if (pData != NULL)
            {
                const BACKEND_STATUS_t status = backendHandle(pData, dataLen);
                pWifiData->backendStatus = status;
                switch (status)
                {
                    case BACKEND_STATUS_OKAY:
                        break;
                    case BACKEND_STATUS_FAIL:
                    case BACKEND_STATUS_RECONNECT:
                        esp_http_client_close(pEvent->client);
                        break;
                }
            }
            break;
        }
        case HTTP_EVENT_ON_FINISH:
            DEBUG("wifi: http finished");
            break;
        case HTTP_EVENT_DISCONNECTED:
            INFO("wifi: http disconnected");
            backendDisconnect();
            break;
    }
    return ESP_OK;
}

/* *********************************************************************************************** */

// the state of the wifi (network) connection
typedef enum WIFI_STATE_e
{
    WIFI_STATE_WAITCFG = 0, // waiting for configuration
    WIFI_STATE_OFFLINE,     // offline --> wait for station connect
    WIFI_STATE_ONLINE,      // station online --> connect to backend
    WIFI_STATE_BACKEND,   // backend connected
    WIFI_STATE_FAIL,        // failure (e.g. connection lost) --> initialise
} WIFI_STATE_t;

static const char *sWifiStateStr(const WIFI_STATE_t state)
{
    switch (state)
    {
        case WIFI_STATE_WAITCFG:  return "WAITCFG";
        case WIFI_STATE_OFFLINE:  return "OFFLINE";
        case WIFI_STATE_ONLINE:   return "ONLINE";
        case WIFI_STATE_BACKEND:  return "BACKEND";
        case WIFI_STATE_FAIL:     return "FAIL";
    }
    return "???";
}

SemaphoreHandle_t sWifiStaConnectedSem;
SemaphoreHandle_t sWifiStaGotIpSem;
static bool sWifiStationOnline;
static uint32_t sWifiConnectCount;

// -------------------------------------------------------------------------------------------------

static WIFI_STATE_t sWifiState;

static void sWifiTask(void *pArg)
{
    UNUSED(pArg);

    static WIFI_DATA_t sWifiData;
    WIFI_STATE_t oldState = WIFI_STATE_OFFLINE;

    while (true)
    {
        if (oldState != sWifiState)
        {
            DEBUG("wifi: %s -> %s", sWifiStateStr(oldState), sWifiStateStr(sWifiState));
            oldState = sWifiState;
        }

        switch (sWifiState)
        {
            // wait until we have a the configuration for the access point
            case WIFI_STATE_WAITCFG:
            {
                if (sWifiData.valid)
                {
                    memset(&sWifiData, 0, sizeof(sWifiData));
                }
                const char *ssid = envGet("ssid");
                const char *pass = envGet("pass");
                if ( (ssid != NULL) && (pass != NULL) && (strlen(ssid) > 0) && (strlen(pass) > 0) )
                {
                    snprintf((char *)sWifiData.ssid, sizeof(sWifiData.ssid), ssid);
                    sWifiData.valid = true;

                    // setup wifi station
                    wifi_config_t staConfig;
                    snprintf((char *)staConfig.sta.ssid, sizeof(staConfig.sta.ssid), ssid);
                    if (strcmp(pass, "none") != 0)
                    {
                        snprintf((char *)staConfig.sta.password, sizeof(staConfig.sta.password), pass);
                    }
                    if (esp_wifi_set_config(ESP_IF_WIFI_STA, &staConfig) != ESP_OK)
                    {
                        sWifiState = WIFI_STATE_FAIL;
                        break;
                    }
                    //if (esp_wifi_start() != ESP_OK)
                    //{
                    //    sWifiState = WIFI_STATE_FAIL;
                    //    break;
                    //}
                    sWifiState = WIFI_STATE_OFFLINE;
                }
                break;
            }

            // connect to access point, go online
            case WIFI_STATE_OFFLINE:
                INFO("wifi: connecting to %s", envGet("ssid"));
                statusNoise(STATUS_NOISE_ABORT);
                statusLed(STATUS_LED_UPDATE);

                // initiate connection to AP
                xSemaphoreTake(sWifiStaConnectedSem, 0);
                xSemaphoreTake(sWifiStaGotIpSem, 0);
                if (esp_wifi_connect() != ESP_OK)
                {
                    sWifiState = WIFI_STATE_FAIL;
                    break;
                }
                // wait for SYSTEM_EVENT_STA_CONNECTED
                if (!xSemaphoreTake(sWifiStaConnectedSem, MS2TICKS(5000)))
                {
                    sWifiState = WIFI_STATE_FAIL;
                    break;
                }
                // wait for SYSTEM_EVENT_STA_GOT_IP:
                if (!xSemaphoreTake(sWifiStaGotIpSem, MS2TICKS(5000)))
                {
                    sWifiState = WIFI_STATE_FAIL;
                    break;
                }
                // we should be online now
                sWifiState = WIFI_STATE_ONLINE;
                break;

            // wait for backend configuration
            case WIFI_STATE_ONLINE:
            {
                const char *backend = envGet("backend");
                if ( (backend != NULL) && (strlen(backend) > 0) )
                {
                    strncpy(sWifiData.backend, backend, sizeof(sWifiData.backend));
                    // make request query parameters
                    getSystemName(sWifiData.name, sizeof(sWifiData.name));
                    tcpip_adapter_ip_info_t ipInfo;
                    if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo) != ESP_OK)
                    {
                        ipInfo.ip.addr = 0;
                    }
                    strncpy(sWifiData.client, getSystemId(), sizeof(sWifiData.client));
                    snprintf(sWifiData.params, sizeof(sWifiData.params), BACKEND_QUERY,
                        BACKEND_ARGS( sWifiData.client, sWifiData.name, sWifiData.ssid, IP2STR(&ipInfo.ip) ) );

                    // create http client handle
                    esp_http_client_config_t clientConfig =
                    {
                        .url           = sWifiData.backend,
                        .method        = HTTP_METHOD_POST,
                        .event_handler = sWifiHttpEventHandler,
                        .auth_type     = HTTP_AUTH_TYPE_BASIC, // preempt auth
                        .timeout_ms    = 10000, // heartbeat is at 5s
                        .user_data     = &sWifiData,
                    };
                    sWifiData.pHttpClient = esp_http_client_init(&clientConfig);
                    if (sWifiData.pHttpClient == NULL)
                    {
                        ERROR("wifi: client fail");
                        sWifiState = WIFI_STATE_FAIL;
                        break;
                    }
                    {
                        const esp_err_t err = esp_http_client_set_post_field(sWifiData.pHttpClient, sWifiData.params, strlen(sWifiData.params));
                        if (err != ESP_OK)
                        {
                            ERROR("wifi: query param fail: %s", esp_err_to_name(err));
                            esp_http_client_cleanup(sWifiData.pHttpClient);
                            sWifiState = WIFI_STATE_FAIL;
                            break;
                        }
                    }
                    {
                        const esp_err_t err = esp_http_client_set_header(sWifiData.pHttpClient, "User-Agent", "gitta/"FF_VERSION);
                        if (err != ESP_OK)
                        {
                            WARNING("wifi: set agent fail: %s", esp_err_to_name(err));
                        }
                    }

                    // we can now connect
                    INFO("wifi: connecting to %s", sWifiData.backend);
                    DEBUG("wifi: params=%s", sWifiData.params);
                    sWifiConnectCount++;
                    sWifiState = WIFI_STATE_BACKEND;
                }
                break;
            }

            // connect to backend and handle incoming data
            case WIFI_STATE_BACKEND:
            {
                // initiate request, this will block until the backend or something else disconnects us
                // received data is handled in sWifiHttpEventHandler()
                const esp_err_t err = esp_http_client_perform(sWifiData.pHttpClient);
                const int status = esp_http_client_get_status_code(sWifiData.pHttpClient);
                DEBUG("wifi: after request: %s, status %d, backend %d", esp_err_to_name(err), status,
                    sWifiData.backendStatus);

                // cleanup
                esp_http_client_cleanup(sWifiData.pHttpClient);

                // probably a network interruption or the remote server disconnecting us
                if ( ((err == ESP_OK) && (status == 200 /* 200 OK */)) ||
                    (sWifiData.backendStatus = BACKEND_STATUS_RECONNECT) )
                {
                    sWifiState = sWifiStationOnline ? WIFI_STATE_ONLINE : WIFI_STATE_FAIL;
                }
                else
                {
                    sWifiState = WIFI_STATE_FAIL;
                }
                break;
            }

            // something has failed --> wait a bit
            case WIFI_STATE_FAIL:
            {
                static uint32_t lastFail;
                const uint32_t now = osTime();
                int waitTime = (now - lastFail) > (1000 * CONFIG_FF_WIFI_STABLE_CONN_THRS) ?
                    CONFIG_FF_WIFI_RECONNECT_INTERVAL : CONFIG_FF_WIFI_RECONNECT_INTERVAL_SLOW;
                lastFail = now;
                statusNoise(STATUS_NOISE_FAIL);
                statusLed(STATUS_LED_FAIL);
                INFO("wifi: failure... waiting %us", waitTime);
                while (waitTime > 0)
                {
                    osSleep(1000);
                    if ( (waitTime < 10) || ((waitTime % 10) == 0) )
                    {
                        DEBUG("wifi: wait... %d", waitTime);
                    }
                    if (waitTime <= 3)
                    {
                        statusNoise(STATUS_NOISE_TICK);
                    }
                    waitTime--;
                }

                // try reconnecting to backend if still online, start over otherwise
                if (sWifiStationOnline)
                {
                    sWifiState = WIFI_STATE_ONLINE;
                }
                else
                {
                    //esp_wifi_stop();
                    sWifiState = WIFI_STATE_WAITCFG;
                }
                break;
            }
        }
        osSleep(100);
    }

}


/* *********************************************************************************************** */

static esp_err_t sWifiEventHandler(void *ctx, system_event_t *event)
{
    UNUSED(ctx);
    switch (event->event_id)
    {
        case SYSTEM_EVENT_STA_CONNECTED:
        {
            system_event_sta_connected_t *pkInfo = &event->event_info.connected;
            INFO("wifi: station connected to %s ("MACSTR", ch %u, auth %s)",
                pkInfo->ssid, MAC2STR(pkInfo->bssid), pkInfo->channel, wifiAuthModeStr(pkInfo->authmode));
            xSemaphoreGive(sWifiStaConnectedSem); // wake waiting sWifiTask()
            break;
        }
        case SYSTEM_EVENT_STA_GOT_IP:
        {
            system_event_sta_got_ip_t *pkInfo = &event->event_info.got_ip;
            INFO("wifi: station got address "IPSTR"/"IPSTR" (gw "IPSTR")",
                IP2STR(&pkInfo->ip_info.ip), IP2STR(&pkInfo->ip_info.netmask), IP2STR(&pkInfo->ip_info.gw));
            sWifiStationOnline = true;
            xSemaphoreGive(sWifiStaGotIpSem); // wake waiting sWifiTask()
            break;
        }
        case SYSTEM_EVENT_STA_DISCONNECTED:
            WARNING("wifi: station disconnected");
            sWifiStationOnline = false;
            break;

        // FIXME: does this ever happen?
        case SYSTEM_EVENT_STA_LOST_IP:
            WARNING("wifi: lost IP");
            sWifiStationOnline = false;
            break;

        case SYSTEM_EVENT_WIFI_READY:
        case SYSTEM_EVENT_SCAN_DONE:
        case SYSTEM_EVENT_STA_START:
        case SYSTEM_EVENT_STA_STOP:
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
        case SYSTEM_EVENT_AP_START:
        case SYSTEM_EVENT_AP_STOP:
        case SYSTEM_EVENT_AP_STACONNECTED:
        case SYSTEM_EVENT_AP_STADISCONNECTED:
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
        case SYSTEM_EVENT_GOT_IP6:
        case SYSTEM_EVENT_ETH_START:
        case SYSTEM_EVENT_ETH_STOP:
        case SYSTEM_EVENT_ETH_CONNECTED:
        case SYSTEM_EVENT_ETH_DISCONNECTED:
        case SYSTEM_EVENT_ETH_GOT_IP:
        case SYSTEM_EVENT_MAX:
            break;

    }
    return ESP_OK;
}

bool wifiScan(const char *prefix)
{
    DEBUG("wifi: start scan");
    if (prefix != NULL)
    {
        PRINT("%s start", prefix);
    }
    esp_err_t err;
    esp_wifi_scan_stop();
    wifi_scan_config_t scanConfig = { .show_hidden = true, /*.scan_type = WIFI_SCAN_TYPE_PASSIVE*/ };
    err = esp_wifi_scan_start(&scanConfig, true);
    if ( err != ESP_OK )
    {
        WARNING("wifi: scan failed: %s", esp_err_to_name(err));
        return false;
    }

    uint16_t numAp;
    err = esp_wifi_scan_get_ap_num(&numAp);
    if (err != ESP_OK )
    {
        return false;
    }
    DEBUG("wifi: done scan (%u ap) %u", numAp, sizeof(wifi_ap_record_t));
    if (numAp > 50)
    {
        WARNING("wifi: too man access points");
        return false;
    }
    wifi_ap_record_t *pApRecs = malloc(numAp * sizeof(wifi_ap_record_t));
    if (pApRecs == NULL)
    {
        WARNING("wifi: malloc");
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
        char str[128];
        snprintf(str, sizeof(str),
            "%s ap %02d/%02d "MACSTR" ch %2u%c rssi %3d %-12s %-9s %-9s %c%c%c %c %s"/* %c%c%c */" %s",
            prefix == NULL ? "" : prefix, ix + 1, numAp,
            MAC2STR(pkAp->bssid),
            pkAp->primary, pkAp->second == WIFI_SECOND_CHAN_ABOVE ? '^' : (pkAp->second == WIFI_SECOND_CHAN_BELOW ? '_' : '-'),
            pkAp->rssi, wifiAuthModeStr(pkAp->authmode), wifiCipherTypeStr(pkAp->pairwise_cipher),
            wifiCipherTypeStr(pkAp->group_cipher),
            (pkAp->phy_11b ? 'B' : '.'), (pkAp->phy_11g ? 'G' : '.'), (pkAp->phy_11n ? 'N' : '.'),
            (pkAp->wps ? 'W' : '.'), country, /*pkAp->country.cc[0], pkAp->country.cc[1], pkAp->country.cc[2],*/
            (pkAp->ssid[0] == 0 ? "<hidden>" : (const char *)pkAp->ssid));
        if (prefix == NULL)
        {
            INFO("wifi:%s", str);
        }
        else
        {
            PRINT("%s", str);
        }
    }
    if (prefix != NULL)
    {
        PRINT("%s done", prefix);
    }
    free(pApRecs);
    return true;
}

/* *********************************************************************************************** */

static void sWifiMonFunc(void)
{
    const char *name;
    const esp_err_t res = tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &name);

    DEBUG("mon: wifi: name=%s state=%s count=%u",
        res == ESP_OK ? name : "<unknown>", sWifiStateStr(sWifiState), sWifiConnectCount);
}


void wifiStart(void)
{
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    char name[32];
    getSystemName(name, sizeof(name));

    INFO("wifi: init (sta mac="MACSTR" name=%s)", MAC2STR(mac), name);

    statusLed(STATUS_LED_FAIL);

    // initialise networking
    tcpip_adapter_init();

    // initialise wifi, start wifi task
    const wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&wifiCfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    //ESP_ERROR_CHECK( esp_wifi_set_auto_connect(false) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    //ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_event_loop_init(sWifiEventHandler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, name) );

    monRegisterMonFunc(sWifiMonFunc);
    monRegisterMonFunc(backendMonStatus);

    // semaphores
    static StaticSemaphore_t sSem1;
    sWifiStaConnectedSem = xSemaphoreCreateBinaryStatic(&sSem1);
    static StaticSemaphore_t sSem2;
    sWifiStaGotIpSem = xSemaphoreCreateBinaryStatic(&sSem2);

    // start wifi task
    static StackType_t sWifiTaskStack[8192 / sizeof(StackType_t)];
    static StaticTask_t sWifiTaskTCB;
    xTaskCreateStaticPinnedToCore(sWifiTask, CONFIG_FF_NAME"_wifi", NUMOF(sWifiTaskStack),
        NULL, CONFIG_FF_WIFI_PRIO, sWifiTaskStack, &sWifiTaskTCB, 1);
}


// eof
