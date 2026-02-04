#include "osal/osal_port.h"
#include "osal/osal_thread.h"
#include "osal/osal_time.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "task.h"

#if (AWLF_OSAL_PORT == OSAL_PORT_FREERTOS)
uint32_t osal_time_ms(void)
{
    return (uint32_t)osal_time_ms64();
}

uint64_t osal_time_ms64(void)
{
    static uint32_t last_tick = 0;
    static uint64_t tick64 = 0;
    static int initialized = 0;

    uint32_t tick = osal_in_isr() ? (uint32_t)xTaskGetTickCountFromISR() : (uint32_t)xTaskGetTickCount();

    if (osal_in_isr())
    {
        UBaseType_t mask = portSET_INTERRUPT_MASK_FROM_ISR();
        if (!initialized)
        {
            initialized = 1;
            last_tick = tick;
            tick64 = (uint64_t)tick;
        }
        else
        {
            if (tick < last_tick)
                tick64 += (1ULL << 32);
            tick64 = (tick64 & ~0xFFFFFFFFULL) | tick;
            last_tick = tick;
        }
        portCLEAR_INTERRUPT_MASK_FROM_ISR(mask);
    }
    else
    {
        taskENTER_CRITICAL();
        if (!initialized)
        {
            initialized = 1;
            last_tick = tick;
            tick64 = (uint64_t)tick;
        }
        else
        {
            if (tick < last_tick)
                tick64 += (1ULL << 32);
            tick64 = (tick64 & ~0xFFFFFFFFULL) | tick;
            last_tick = tick;
        }
        taskEXIT_CRITICAL();
    }

    return osal_ticks_to_ms(tick64);
}

uint32_t osal_time_us(void)
{
    return (uint32_t)osal_time_us64();
}

uint64_t osal_time_us64(void)
{
    /* FreeRTOS默认以tick为基础，微秒分辨率受tick限制 */
    uint64_t ms = osal_time_ms64();
    return ms * OSAL_US_PER_MS;
}

uint64_t osal_time_ns64(void)
{
    /* FreeRTOS默认以tick为基础，纳秒分辨率受tick限制 */
    uint64_t ms = osal_time_ms64();
    return ms * OSAL_NS_PER_MS;
}

void osal_delay_until_ms(uint32_t* last_ms, uint32_t period_ms)
{
    if (!last_ms)
        return;

    uint32_t now = osal_time_ms();
    uint32_t next = *last_ms + period_ms;
    if ((int32_t)(next - now) > 0)
        osal_thread_sleep_ms(next - now);
    *last_ms = next;
}
#endif /* AWLF_OSAL_PORT == OSAL_PORT_FREERTOS */
