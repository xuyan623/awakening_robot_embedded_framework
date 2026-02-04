#include "awlib.h"
#include "osal/osal_core.h"
#include "sync/completion.h"

#if (AWLF_OSAL_PORT == OSAL_PORT_FREERTOS) && (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)

#include "FreeRTOS.h"
#include "task.h"

#if !defined(AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX)
#define AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX 0u
#endif

#if (AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX != 0u)
#ifndef configTASK_NOTIFICATION_ARRAY_ENTRIES
#error "使用非 0 的 AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX 需要定义 configTASK_NOTIFICATION_ARRAY_ENTRIES"
#endif
#if (configTASK_NOTIFICATION_ARRAY_ENTRIES <= AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX)
#error "configTASK_NOTIFICATION_ARRAY_ENTRIES 过小，无法满足 AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX"
#endif
#endif

AwlfRet_e completion_accel_init(Completion_t completion)
{
    if (!completion)
    {
        return AWLF_ERROR_NULL;
    }

    completion->wait_thread = NULL;
    completion->status = COMP_INIT;

    return AWLF_OK;
}

void completion_accel_deinit(Completion_t completion)
{
    if (!completion)
    {
        return;
    }

    completion->wait_thread = NULL;
    completion->status = COMP_INIT;
}

AwlfRet_e completion_accel_wait(Completion_t completion, size_t timeout_ms)
{
    if (!completion)
    {
        return AWLF_ERROR_NULL;
    }
    if (osal_in_isr())
    {
        return AWLF_ERROR_PARAM;
    }

    osal_thread_t self = osal_thread_self();
    if (!self)
    {
        return AWLF_ERROR;
    }

    uint32_t irq_state = osal_irq_save();
    if (completion->status == COMP_WAIT)
    {
        osal_irq_restore(irq_state);
        return AWLF_ERROR_BUSY;
    }
    if (completion->status == COMP_DONE)
    {
        completion->status = COMP_INIT;
        completion->wait_thread = NULL;
        osal_irq_restore(irq_state);
        return AWLF_OK;
    }
    if (timeout_ms == 0U)
    {
        osal_irq_restore(irq_state);
        return AWLF_ERROR_TIMEOUT;
    }
    if (completion->wait_thread != NULL)
    {
        osal_irq_restore(irq_state);
        return AWLF_ERROR_BUSY;
    }

    completion->wait_thread = self;
    completion->status = COMP_WAIT;
    osal_irq_restore(irq_state);

    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    uint32_t value = ulTaskNotifyTakeIndexed((UBaseType_t)AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX, pdTRUE, ticks);

    irq_state = osal_irq_save();
    if (value == 0U && completion->status != COMP_DONE)
    {
        completion->wait_thread = NULL;
        completion->status = COMP_INIT;
        osal_irq_restore(irq_state);
        return AWLF_ERROR_TIMEOUT;
    }

    completion->wait_thread = NULL;
    completion->status = COMP_INIT;
    osal_irq_restore(irq_state);
    return AWLF_OK;
}

static void completion_accel_give(TaskHandle_t task_handle)
{
    if (osal_in_isr())
    {
        BaseType_t higher_priority_task_woken = pdFALSE;
        vTaskNotifyGiveIndexedFromISR(task_handle, (UBaseType_t)AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
        return;
    }

    (void)xTaskNotifyGiveIndexed(task_handle, (UBaseType_t)AWLF_FREERTOS_COMPLETION_NOTIFY_INDEX);
}

AwlfRet_e completion_accel_done(Completion_t completion)
{
    if (!completion)
    {
        return AWLF_ERROR_NULL;
    }

    uint32_t irq_state = osal_irq_save();
    AwlfRet_e ret = AWLF_ERROR;

    switch (completion->status)
    {
    case COMP_INIT:
        completion->status = COMP_DONE;
        ret = AWLF_OK;
        break;

    case COMP_WAIT:
        if (completion->wait_thread != NULL)
        {
            TaskHandle_t task_handle = (TaskHandle_t)completion->wait_thread;
            completion->wait_thread = NULL;
            completion->status = COMP_DONE;
            osal_irq_restore(irq_state);
            completion_accel_give(task_handle);
            return AWLF_OK;
        }
        ret = AWLF_ERROR_EMPTY;
        break;

    case COMP_DONE:
        ret = AWLF_ERROR_BUSY;
        break;

    default:
        ret = AWLF_ERROR;
        break;
    }

    osal_irq_restore(irq_state);
    return ret;
}

#endif /* AWLF_OSAL_PORT == OSAL_PORT_FREERTOS && AWLF_SYNC_ACCEL && AWLF_SYNC_ACCEL_COMPLETION */



