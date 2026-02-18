#include "osal/osal_mutex.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "semphr.h"
#include "task.h"

static int osal_mutex_check_task_context(void)
{
    int in_isr = osal_is_in_isr();
    OSAL_ASSERT(in_isr == 0);
    return (in_isr == 0);
}

static int osal_mutex_is_current_owner(osal_mutex_t mutex)
{
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    TaskHandle_t owner_task = xSemaphoreGetMutexHolder((SemaphoreHandle_t)mutex);
    return (owner_task == current_task);
}

osal_status_t osal_mutex_create(osal_mutex_t* mutex)
{
    if (!mutex)
        return OSAL_INVALID;
    if (!osal_mutex_check_task_context())
        return OSAL_INVALID;

    *mutex = (osal_mutex_t)xSemaphoreCreateMutex();
    return (*mutex) ? OSAL_OK : OSAL_NO_RESOURCE;
}

osal_status_t osal_mutex_delete(osal_mutex_t mutex)
{
    if (!mutex)
        return OSAL_INVALID;
    if (!osal_mutex_check_task_context())
        return OSAL_INVALID;

    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return OSAL_OK;
}

osal_status_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    if (!mutex)
        return OSAL_INVALID;
    if (!osal_mutex_check_task_context())
        return OSAL_INVALID;

    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    BaseType_t ok = xSemaphoreTake((SemaphoreHandle_t)mutex, ticks);
    return osal_wait_result_to_status(ok, timeout_ms);
}

osal_status_t osal_mutex_unlock(osal_mutex_t mutex)
{
    if (!mutex)
        return OSAL_INVALID;
    if (!osal_mutex_check_task_context())
        return OSAL_INVALID;

    OSAL_ASSERT_IN_TASK();
    if (!osal_mutex_is_current_owner(mutex))
        return OSAL_INVALID;

    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdPASS) ? OSAL_OK : OSAL_INVALID;
}
