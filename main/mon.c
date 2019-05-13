/*!
    \file
    \brief GITTA Tschenggins Lämpli: system monitor (see \ref FF_MON)

    - Copyright (c) 2018 Philippe Kehl & flipflip industries (flipflip at oinkzwurgl dot org),
      https://oinkzwurgl.org/projaeggd/tschenggins-laempli
*/

#include "stdinc.h"
#include "debug.h"
#include "stuff.h"
#include "env.h"
#include "mon.h"

#define MON_OFFSET  1000
#define MAX_TASKS  25

static volatile uint32_t svMonIsrStart;
static volatile uint32_t svMonIsrCount;
static volatile uint32_t svMonIsrTime;

//void monIsrEnter(void)
//{
//    svMonIsrStart = RTC.COUNTER;
//    svMonIsrCount++;
//}

//void monIsrLeave(void)
//{
//    svMonIsrTime += RTC.COUNTER - svMonIsrStart;
//}

static int sTaskSortFunc(const void *a, const void *b)
{
    //return (int)((const TaskStatus_t *)a)->xTaskNumber - (int)((const TaskStatus_t *)b)->xTaskNumber;

    const TaskStatus_t *pA = (const TaskStatus_t *)a;
    const TaskStatus_t *pB = (const TaskStatus_t *)b;
    return (pA->xCoreID != pB->xCoreID) ?
        ((int)pA->xCoreID - (int)pB->xCoreID) :
      //((int)pA->xTaskNumber - (int)pB->xTaskNumber);
        ((int)pB->uxBasePriority - (int)pA->uxBasePriority);


}

static MON_FUNC_t sMonFuncs[10] = { 0 };
static int sMonFuncsNum = 0;

bool monRegisterMonFunc(MON_FUNC_t monFunc)
{
    if (sMonFuncsNum < (int)(NUMOF(sMonFuncs)))
    {
        sMonFuncs[sMonFuncsNum] = monFunc;
        sMonFuncsNum++;
        return true;
    }
    else
    {
        return false;
    }
}

static uint32_t sMonTick;
static uint32_t sMonPeriod;

void monSetPeriod(const char *period)
{
    if (period != NULL)
    {
        double periodMs = atof(period);
        periodMs = floor( MAX(0, periodMs) * 1000.0 );

        sMonTick = xTaskGetTickCount();
        sMonPeriod = MS2TICKS((uint32_t)periodMs);
    }
    if ( (sMonPeriod > 0) && (sMonPeriod < 500) )
    {
        sMonPeriod = 500;
    }
    DEBUG("mon: period=%u", sMonPeriod);
}

static void sMonTask(void *pArg)
{
    UNUSED(pArg);
    TaskStatus_t *pTasks = NULL;

    while (true)
    {
        if (pTasks != NULL)
        {
            free(pTasks);
            pTasks = NULL;
        }
        // wait until it's time to dump the status (0 = monitor off)
        if (sMonPeriod > 0)
        {
            static uint32_t sMonLastTick;
            vTaskDelayUntil(&sMonTick, MS2TICKS(100));
            //DEBUG("tick %u per %u rem %u", sMonTick, sMonPeriod, (sMonTick % sMonPeriod));
            if ( sMonTick >= (sMonLastTick + sMonPeriod) )
            {
                sMonLastTick = sMonTick;
            }
            else
            {
                continue;
            }
        }
        else
        {
            osSleep(42);
            continue;
        }

        const int nTasks = uxTaskGetNumberOfTasks();
        if (nTasks > MAX_TASKS)
        {
            ERROR("mon: too many tasks");
            continue;
        }

        // allocate memory for tasks status
        const unsigned int allocSize = nTasks * sizeof(TaskStatus_t);
        pTasks = malloc(allocSize);
        if (pTasks == NULL)
        {
            ERROR("mon: malloc");
            continue;
        }
        memset(pTasks, 0, allocSize);

        // get ISR runtime stats
        //uint32_t isrCount, isrTime, isrTotalRuntime;
        //static uint32_t sIsrLastRuntime;
        //CS_ENTER;
        //const uint32_t rtcCounter = RTC.COUNTER;
        //isrTotalRuntime = rtcCounter - sIsrLastRuntime;
        //sIsrLastRuntime = rtcCounter;
        //isrCount = svMonIsrTime;
        //isrTime = svMonIsrCount;
        //svMonIsrCount = 0;
        //svMonIsrTime = 0;
        //CS_LEAVE;

        uint32_t totalRuntime;
        const int nnTasks = uxTaskGetSystemState(pTasks, nTasks, &totalRuntime);
        if (nTasks != nnTasks)
        {
            ERROR("mon: %u != %u", nTasks, nnTasks);
            continue;
        }

        // sort by task ID
        qsort(pTasks, nTasks, sizeof(TaskStatus_t), sTaskSortFunc);

        // total runtime (tasks, OS, ISRs) since we checked last
        static uint32_t sLastTotalRuntime;
        {
            const uint32_t runtime = totalRuntime;
            totalRuntime = totalRuntime - sLastTotalRuntime;
            sLastTotalRuntime = runtime;
        }
        // calculate time spent in each task since we checked last
        static uint32_t sLastRuntimeCounter[MAX_TASKS];
        uint32_t totalRuntimeTasks = 0;
        for (int ix = 0; ix < nTasks; ix++)
        {
            TaskStatus_t *pTask = &pTasks[ix];
            const uint32_t runtime = pTask->ulRunTimeCounter;
            pTask->ulRunTimeCounter = pTask->ulRunTimeCounter - sLastRuntimeCounter[ix];
            sLastRuntimeCounter[ix] = runtime;
            totalRuntimeTasks += pTask->ulRunTimeCounter;
        }

        // FIXME: why?
        if (totalRuntimeTasks > totalRuntime)
        {
            totalRuntime = totalRuntimeTasks;
        }

        // RTC
        //static uint32_t sLastRtc;
        //const uint32_t msss = system_get_time() / 1000; // ms
        //const uint32_t thisRtc = system_get_rtc_time();
        //const uint32_t drtc = /*roundl*/( (double)(sLastRtc ? thisRtc - sLastRtc : 0)
        //    * (1.0/1000.0/4096.0) * system_rtc_clock_cali_proc() ); // us -> ms
        //sLastRtc = thisRtc;

        // print monitor info
        DEBUG("--------------------------------------------------------------------------------");
        DEBUG("mon: sys: ticks=%u"/* msss=%u drtc=%u*/" heap=%u/%u"/* isr=%u (%.2fkHz, %.1f%%) mhz=%u"*/,
            sMonTick, /*msss, drtc,*/ /*xPortGetFreeHeapSize(), */esp_get_free_heap_size(), esp_get_minimum_free_heap_size()/*,
            isrCount,
            (double)isrCount / ((double)MON_PERIOD / 1000.0) / 1000.0,
            (double)isrTime * 100.0 / (double)isrTotalRuntime, system_get_cpu_freq()*/);

        for (int ix = 0; ix < sMonFuncsNum; ix++)
        {
            sMonFuncs[ix]();
        }

        // print tasks info
        for (int ix = 0; ix < nTasks; ix++)
        {
            const TaskStatus_t *pkTask = &pTasks[ix];
            char state = '?';
            switch (pkTask->eCurrentState)
            {
                case eRunning:   state = 'X'; break;
                case eReady:     state = 'R'; break;
                case eBlocked:   state = 'B'; break;
                case eSuspended: state = 'S'; break;
                case eDeleted:   state = 'D'; break;
                //case eInvalid:   state = 'I'; break;
            }
            const char core = pkTask->xCoreID == tskNO_AFFINITY ? '*' : ('0' + pkTask->xCoreID);

            char perc[8];
            if (pkTask->ulRunTimeCounter)
            {
                const double p = (double)pkTask->ulRunTimeCounter * 100.0 / (double)totalRuntimeTasks;
                if (p < 0.05)
                {
                    strcpy(perc, "<0.1%");
                }
                else
                {
                    snprintf(perc, sizeof(perc), "%5.1f%%", p);
                }
            }
            else
            {
                strcpy(perc, "0.0%");
            }
            DEBUG("mon: tsk: %02d %-16s %c %c %2d-%2d %4u %6s",
                (int)pkTask->xTaskNumber, pkTask->pcTaskName, state, core,
                (int)pkTask->uxCurrentPriority, (int)pkTask->uxBasePriority,
                pkTask->usStackHighWaterMark, perc);
        }
        DEBUG("--------------------------------------------------------------------------------");
    }
}

void monStart(void)
{
    INFO("mon: start");

    monSetPeriod( envGet("monitor") );

    static StackType_t sMonTaskStack[4096 / sizeof(StackType_t)];
    static StaticTask_t sMonTaskTCB;
    xTaskCreateStaticPinnedToCore(sMonTask, CONFIG_FF_NAME"_mon", NUMOF(sMonTaskStack),
        NULL, CONFIG_FF_MON_PRIO, sMonTaskStack, &sMonTaskTCB, 1);
}

// eof
