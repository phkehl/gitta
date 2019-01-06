/*
    \file
    \brief GITTA Tschenggins Lämpli: console (see \ref FF_CONSOLE)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries <flipflip at oinkzwurgl dot org>,
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"

#define FANCY_PROMPT

#include <esp_console.h>
#ifdef FANCY_PROMPT
#  include <esp_vfs_dev.h>
#  include <driver/uart.h>
#  include <linenoise/linenoise.h>
#endif
#include <argtable3/argtable3.h>

#include "debug.h"
#include "stuff.h"
#include "env.h"
#include "wifi.h"
#include "mon.h"
#include "console.h"

#if (CONFIG_CONSOLE_UART_BAUDRATE != CONFIG_MONITOR_BAUD)
#  error Missmatch in console and monitor baudrate (CONFIG_CONSOLE_UART_BAUDRATE != CONFIG_MONITOR_BAUD)!
#endif

/* *********************************************************************************************** */

static int sCmdFreeFunc(int argc, char **argv)
{
    UNUSED(argc);
    PRINT("%s %u %u", argv[0], esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
    return ESP_OK;
}

static void sCmdFreeRegister(void)
{
    const esp_console_cmd_t cmd =
    {
        .command = "free", .hint = NULL, .func = &sCmdFreeFunc,
        .help = "get size of available heap, and lowest size ever available",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static int sCmdRestartFunc(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    PRINT("%s", argv[0]);
    INFO("------------------------------");
    INFO("--- !!! Restarting now !!! ---");
    INFO("------------------------------");
    FLUSH();
    osSleep(1000);
    esp_restart();
    return ESP_OK;
}

static void sCmdRestartRegister(void)
{
    const esp_console_cmd_t cmd =
    {
        .command = "restart", .hint = NULL, .func = &sCmdRestartFunc,
        .help = "restart system",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

// https://www.argtable.org/tutorial/
static struct
{
    struct arg_str *onOff;
    struct arg_end *end;
} sCmdDebugArgs;

static int sCmdDebugFunc(int argc, char **argv)
{
    if ( arg_parse(argc, argv, (void **)&sCmdDebugArgs) != 0 )
    {
        arg_print_errors(stderr, sCmdDebugArgs.end, argv[0]);
        return ESP_ERR_INVALID_ARG;
    }

    const char *argOnOff = sCmdDebugArgs.onOff->sval[0];
    if (debugSetLevel(argOnOff))
    {
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_INVALID_ARG;
    }
}

static void sCmdDebugRegister(void)
{
    //sCmdDebugArgs.onOff = arg_strn(NULL, NULL, "on|off", 1, 1, "enable or disable debug");
    sCmdDebugArgs.onOff = arg_str1(NULL, NULL, "on|off", "enable or disable debug");
    sCmdDebugArgs.end   = arg_end(4);
    const esp_console_cmd_t cmd =
    {
        .command = "debug", .hint = NULL, .func = &sCmdDebugFunc, .argtable = &sCmdDebugArgs,
        .help = "debugging level",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static int sCmdPrintenvFunc(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    envPrint(argv[0]);
    return ESP_OK;
}

static void sCmdPrintenvRegister(void)
{
    const esp_console_cmd_t cmd =
    {
        .command = "printenv", .hint = NULL, .func = &sCmdPrintenvFunc,
        .help = "print all environment variables",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static struct
{
    struct arg_str *key;
    struct arg_end *end;
} sCmdGetenvArgs;

static int sCmdGetenvFunc(int argc, char **argv)
{
    if ( arg_parse(argc, argv, (void **)&sCmdGetenvArgs) != 0 )
    {
        arg_print_errors(stderr, sCmdGetenvArgs.end, argv[0]);
        return ESP_ERR_INVALID_ARG;
    }

    const char *key = sCmdGetenvArgs.key->sval[0];
    const char *val = envGet(key);
    if (val != NULL)
    {
        PRINT("%s %s \"%s\"", argv[0], key, val);
        return ESP_OK;
    }
    else
    {
        PRINT("%s fail", argv[0]);
        return ESP_ERR_INVALID_ARG;
    }
}

static void sCmdGetenvRegister(void)
{
    sCmdGetenvArgs.key = arg_str1(NULL, NULL, "<key>", "name of key");
    sCmdGetenvArgs.end   = arg_end(4);
    const esp_console_cmd_t cmd =
    {
        .command = "getenv", .hint = NULL, .func = &sCmdGetenvFunc, .argtable = &sCmdGetenvArgs,
        .help = "get environment variable",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static struct
{
    struct arg_str *key;
    struct arg_str *val;
    struct arg_end *end;
} sCmdSetenvArgs;

static int sCmdSetenvFunc(int argc, char **argv)
{
    if ( arg_parse(argc, argv, (void **)&sCmdSetenvArgs) != 0 )
    {
        arg_print_errors(stderr, sCmdSetenvArgs.end, argv[0]);
        return ESP_ERR_INVALID_ARG;
    }

    const char *key = sCmdSetenvArgs.key->sval[0];
    const char *val = sCmdSetenvArgs.val->sval[0];

    if (envSet(key, val))
    {
        PRINT("%s %s \"%s\"", argv[0], key, val);
        return ESP_OK;
    }
    else
    {
        PRINT("%s fail", argv[0]);
        return ESP_ERR_INVALID_ARG;
    }
}

static void sCmdSetenvRegister(void)
{
    sCmdSetenvArgs.key = arg_str1(NULL, NULL, "<key>", "name of key");
    sCmdSetenvArgs.val = arg_str1(NULL, NULL, "<val>", "value of variable");
    sCmdSetenvArgs.end   = arg_end(4);
    const esp_console_cmd_t cmd =
    {
        .command = "setenv", .hint = NULL, .func = &sCmdSetenvFunc, .argtable = &sCmdSetenvArgs,
        .help = "set environment variable",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static int sCmdScanFunc(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);
    return wifiScan("scan") ? ESP_OK : ESP_FAIL;
}

static void sCmdScanRegister(void)
{
    const esp_console_cmd_t cmd =
    {
        .command = "scan", .hint = NULL, .func = &sCmdScanFunc,
        .help = "scan for wifi access points",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

// -------------------------------------------------------------------------------------------------

static struct
{
    struct arg_str *period;
    struct arg_end *end;
} sCmdMonitorArgs;

static int sCmdMonitorFunc(int argc, char **argv)
{
    if ( arg_parse(argc, argv, (void **)&sCmdMonitorArgs) != 0 )
    {
        arg_print_errors(stderr, sCmdMonitorArgs.end, argv[0]);
        return ESP_ERR_INVALID_ARG;
    }

    const char *argPeriod = sCmdMonitorArgs.period->sval[0];
    monSetPeriod(argPeriod);

    return ESP_OK;
}

static void sCmdMonitorRegister(void)
{
    //sCmdMonitorArgs.onOff = arg_strn(NULL, NULL, "on|off", 1, 1, "enable or disable monitor");
    sCmdMonitorArgs.period = arg_str1(NULL, NULL, "<time>", "set monitor period");
    sCmdMonitorArgs.end    = arg_end(4);
    const esp_console_cmd_t cmd =
    {
        .command = "monitor", .hint = NULL, .func = &sCmdMonitorFunc, .argtable = &sCmdMonitorArgs,
        .help = "system monitorg period",
    };
    ASSERT_ESP_OK( esp_console_cmd_register(&cmd) );
}

/* *********************************************************************************************** */

#define MAX_CMD_LEN 256

static void sConsoleTask(void *pArg)
{
    UNUSED(pArg);
    //esp_vfs_dev_uart_use_nonblocking(CONFIG_CONSOLE_UART_NUM);
    //esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);

#ifdef FANCY_PROMPT
    //const char *prompt = LOG_COLOR_I CONFIG_FF_NAME "> " LOG_RESET_COLOR;
    const char *prompt = LOG_BOLD(LOG_COLOR_CYAN) CONFIG_FF_NAME "> " LOG_RESET_COLOR;
    if (linenoiseProbe())
    {
        WARNING("console: dumb terminal detected; use PuTTY, picocom, GNU screen or similar");
        linenoiseSetDumbMode(1);
#  if CONFIG_LOG_COLORS
        prompt = CONFIG_FF_NAME "> ";
#  endif
    }
#else
    static char userInput[MAX_CMD_LEN];
    char *pUserInput = userInput;
#endif

    INFO("console: ready, say 'help' for help");

    while (true)
    {
#ifdef FANCY_PROMPT
        char *userInput = linenoise(prompt);
        if (userInput == NULL)
        {
            continue;
        }
        linenoiseHistoryAdd(userInput);
        {
            {
#else
        const int inputLen = strlen(userInput);
        const int remInput = sizeof(userInput) - inputLen;

        // input buffer full, discard all input
        if (remInput <= 1)
        {
            WARNING("console: too much input, discarding");
            userInput[0] = '\0';
            pUserInput = userInput;
        }
        // read more user input (this will non-blocking read none, one or multiple bytes)
        else if (fgets(pUserInput, remInput, stdin) != NULL)
        {
            // remove trailing \r and/or \n (and check if we have a full input line)
            char *pInput = &userInput[strlen(userInput)];
            bool haveEol = false;
            while (pInput >= userInput)
            {
                if (*pInput == '\0')
                {
                    // ...
                }
                else if ( (*pInput == '\r') || (*pInput == '\n') )
                {
                    *pInput = '\0';
                    haveEol = true;
                }
                else if (isprint((int)*pInput) == 0)
                {
                    *pInput = ' ';
                }
                pInput--;
            }

            // process input
            if (haveEol)
            {
                DEBUG("console: command [%s] (%d)", userInput, strlen(userInput));
#endif
                // run command
                int ret;
                esp_err_t err = esp_console_run(userInput, &ret);
                if (err == ESP_ERR_NOT_FOUND)
                {
                    ERROR("console: illegal command");
                }
                else if (err == ESP_ERR_INVALID_ARG)
                {
                    ERROR("console: empty command");
                }
                else if (err == ESP_OK)
                {
                    if (ret != ESP_OK)
                    {
                        ERROR("console: command error %s (%d)", esp_err_to_name(ret), ret);
                    }
                    else
                    {
                        DEBUG("console: command success");
                    }
                }
                else
                {
                    ERROR("console: error %s (%d)", esp_err_to_name(err), err);
                }
#ifdef FANCY_PROMPT
                linenoiseFree(userInput);
            }
        }
#else
                // reset input buffer
                userInput[0] = '\0';
                pUserInput = userInput;
            }
            // prepare to receive more input
            else
            {
                pUserInput = &userInput[strlen(userInput)];
            }
        }
        // no input at the moment, sleep a bit
        else
        {
            osSleep(10);
        }
#endif
    }
}

void consoleStart(void)
{
    INFO("console: start");

    // initialize the console
    esp_console_config_t consoleConfig =
    {
        .max_cmdline_args   = 8,
        .max_cmdline_length = MAX_CMD_LEN,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ASSERT_ESP_OK( esp_console_init(&consoleConfig) );

    // initialise linenoise prompt thingy
#ifdef FANCY_PROMPT
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
    linenoiseSetMultiLine(1);
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
    linenoiseHistorySetMaxLen(100);
    //const uart_config_t uart_config = {
    //        .baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
    //        .data_bits = UART_DATA_8_BITS,
    //        .parity = UART_PARITY_DISABLE,
    //        .stop_bits = UART_STOP_BITS_1,
    //        .use_ref_tick = true
    //};
    //ASSERT_ESP_OK( uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config) );
    ASSERT_ESP_OK( uart_driver_install(CONFIG_CONSOLE_UART_NUM, MAX_CMD_LEN, 0, 0, NULL, 0) );
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
#endif

    // register commands
    esp_console_register_help_command();
    sCmdFreeRegister();
    sCmdDebugRegister();
    sCmdRestartRegister();
    sCmdPrintenvRegister();
    sCmdGetenvRegister();
    sCmdSetenvRegister();
    sCmdScanRegister();
    sCmdMonitorRegister();

    // start console task
    static StackType_t sConsoleTaskStack[4096 / sizeof(StackType_t)];
    static StaticTask_t sConsoleTaskTCB;
    xTaskCreateStaticPinnedToCore(sConsoleTask, CONFIG_FF_NAME"_console", NUMOF(sConsoleTaskStack),
        NULL, CONFIG_FF_CONSOLE_PRIO, sConsoleTaskStack, &sConsoleTaskTCB, 1);
}

/* *********************************************************************************************** */
// eof
