#ifndef __AWLF_OSAL_TIME_FREERTOS_H__
#define __AWLF_OSAL_TIME_FREERTOS_H__

#include <stdint.h>

#include "FreeRTOS.h"
#include "osal/osal_core.h"

static inline TickType_t osal_ms_to_ticks(uint32_t ms)
{
    if (ms == OSAL_WAIT_FOREVER)
        return portMAX_DELAY;
    if (ms == 0U)
        return 0;
    uint64_t ticks = ((uint64_t)ms * (uint64_t)configTICK_RATE_HZ + 999ULL) / 1000ULL;
    if (ticks > (uint64_t)portMAX_DELAY)
        return portMAX_DELAY;
    return (TickType_t)ticks;
}

static inline uint64_t osal_ticks_to_ms(uint64_t ticks)
{
    return (ticks * 1000ULL) / (uint64_t)configTICK_RATE_HZ;
}

#endif
