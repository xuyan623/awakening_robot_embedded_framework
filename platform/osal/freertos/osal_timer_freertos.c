#include "osal/osal_timer.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "timers.h"

osal_timer_t osal_timer_create(const char* name, uint32_t period_ms, int auto_reload, void* user_id, osal_timer_cb_t cb)
{
    if (!cb || period_ms == 0U)
        return NULL;
    return (osal_timer_t)xTimerCreate(name, osal_ms_to_ticks(period_ms), auto_reload ? pdTRUE : pdFALSE, user_id,
                                      (TimerCallbackFunction_t)cb);
}

int osal_timer_start(osal_timer_t timer, uint32_t timeout_ms)
{
    if (!timer)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    return xTimerStart((TimerHandle_t)timer, osal_ms_to_ticks(timeout_ms)) == pdPASS ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_timer_stop(osal_timer_t timer, uint32_t timeout_ms)
{
    if (!timer)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    return xTimerStop((TimerHandle_t)timer, osal_ms_to_ticks(timeout_ms)) == pdPASS ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_timer_delete(osal_timer_t timer, uint32_t timeout_ms)
{
    if (!timer)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    return xTimerDelete((TimerHandle_t)timer, osal_ms_to_ticks(timeout_ms)) == pdPASS ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

void* osal_timer_get_id(osal_timer_t timer)
{
    return pvTimerGetTimerID((TimerHandle_t)timer);
}

void osal_timer_set_id(osal_timer_t timer, void* id)
{
    vTimerSetTimerID((TimerHandle_t)timer, id);
}
