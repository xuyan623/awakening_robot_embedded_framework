#include "osal/osal_sem.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "semphr.h"

int osal_sem_create(osal_sem_t* sem, uint32_t max_count, uint32_t init_count)
{
    if (!sem || init_count > max_count)
        return OSAL_ERR_PARAM;
    *sem = (osal_sem_t)xSemaphoreCreateCounting((UBaseType_t)max_count, (UBaseType_t)init_count);
    return (*sem) ? OSAL_OK : OSAL_ERR_NOMEM;
}

int osal_sem_delete(osal_sem_t sem)
{
    if (!sem)
        return OSAL_ERR_PARAM;
    vSemaphoreDelete((SemaphoreHandle_t)sem);
    return OSAL_OK;
}

int osal_sem_wait(osal_sem_t sem, uint32_t timeout_ms)
{
    if (!sem)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    return (xSemaphoreTake((SemaphoreHandle_t)sem, ticks) == pdPASS) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_sem_post(osal_sem_t sem)
{
    if (!sem)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    return (xSemaphoreGive((SemaphoreHandle_t)sem) == pdPASS) ? OSAL_OK : OSAL_ERR;
}

int osal_sem_post_isr(osal_sem_t sem)
{
    if (!sem)
        return OSAL_ERR_PARAM;
    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xSemaphoreGiveFromISR((SemaphoreHandle_t)sem, &need_switch);
    if (need_switch)
        portYIELD_FROM_ISR(need_switch);
    return (ok == pdPASS) ? OSAL_OK : OSAL_ERR;
}
