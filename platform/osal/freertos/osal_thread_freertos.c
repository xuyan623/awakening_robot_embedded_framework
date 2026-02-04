#include "osal/osal_thread.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "task.h"

int osal_thread_create(osal_thread_t* thread, const osal_thread_attr_t* attr, osal_thread_entry_t entry, void* arg)
{
    TaskHandle_t handle = NULL;
    const char* name = (attr && attr->name) ? attr->name : "osal_thread";
    uint32_t stack_size = (attr && attr->stack_size) ? attr->stack_size : 512;
    uint32_t prio = (attr) ? attr->priority : 1;

    if (!entry)
        return OSAL_ERR_PARAM;
    if (xTaskCreate((TaskFunction_t)entry, name, (uint16_t)stack_size, arg, (UBaseType_t)prio, &handle) == pdPASS)
    {
        if (thread)
            *thread = (osal_thread_t)handle;
        return OSAL_OK;
    }
    return OSAL_ERR_NOMEM;
}

osal_thread_t osal_thread_self(void)
{
    return (osal_thread_t)xTaskGetCurrentTaskHandle();
}

void osal_thread_sleep_ms(uint32_t ms)
{
    if (osal_in_isr())
        return;
    OSAL_ASSERT_IN_TASK();
    vTaskDelay(osal_ms_to_ticks(ms));
}

void osal_thread_yield(void)
{
    taskYIELD();
}

void osal_thread_exit(void)
{
    vTaskDelete(NULL);
}

int osal_thread_delete(osal_thread_t thread)
{
    if (!thread)
        return OSAL_ERR_PARAM;
    vTaskDelete((TaskHandle_t)thread);
    return OSAL_OK;
}

int osal_kernel_start(void)
{
    vTaskStartScheduler();
    return OSAL_OK;
}
