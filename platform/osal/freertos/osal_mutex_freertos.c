#include "osal/osal_mutex.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "semphr.h"

int osal_mutex_create(osal_mutex_t* mutex)
{
    if (!mutex)
        return OSAL_ERR_PARAM;
    *mutex = (osal_mutex_t)xSemaphoreCreateMutex();
    return (*mutex) ? OSAL_OK : OSAL_ERR_NOMEM;
}

int osal_mutex_delete(osal_mutex_t mutex)
{
    if (!mutex)
        return OSAL_ERR_PARAM;
    vSemaphoreDelete((SemaphoreHandle_t)mutex);
    return OSAL_OK;
}

int osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms)
{
    if (!mutex)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    return (xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) == pdPASS) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_mutex_unlock(osal_mutex_t mutex)
{
    if (!mutex)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    return (xSemaphoreGive((SemaphoreHandle_t)mutex) == pdPASS) ? OSAL_OK : OSAL_ERR;
}
