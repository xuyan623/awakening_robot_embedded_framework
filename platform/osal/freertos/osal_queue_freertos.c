#include "osal/osal_queue.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "queue.h"

static inline int osal_queue_check_task_context(void)
{
    int in_isr = osal_is_in_isr();
    OSAL_ASSERT(in_isr == 0);
    return (in_isr == 0);
}

static inline int osal_queue_check_isr_context(void)
{
    int in_isr = osal_is_in_isr();
    OSAL_ASSERT(in_isr != 0);
    return (in_isr != 0);
}

static inline osal_status_t osal_queue_from_isr_result_to_status(BaseType_t result)
{
    return (result == pdPASS) ? OSAL_OK : OSAL_WOULD_BLOCK;
}

osal_status_t osal_queue_create(osal_queue_t* queue, uint32_t length, uint32_t item_size)
{
    if (!queue || length == 0u || item_size == 0u)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    QueueHandle_t handle = xQueueCreate((UBaseType_t)length, (UBaseType_t)item_size);
    if (!handle)
        return OSAL_NO_RESOURCE;

    *queue = (osal_queue_t)handle;
    return OSAL_OK;
}

osal_status_t osal_queue_delete(osal_queue_t queue)
{
    if (!queue)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    vQueueDelete((QueueHandle_t)queue);
    return OSAL_OK;
}

osal_status_t osal_queue_send(osal_queue_t queue, const void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    BaseType_t ok = xQueueSend((QueueHandle_t)queue, item, ticks);
    return osal_wait_result_to_status(ok, timeout_ms);
}

osal_status_t osal_queue_send_from_isr(osal_queue_t queue, const void* item)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_isr_context())
        return OSAL_INVALID;

    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xQueueSendFromISR((QueueHandle_t)queue, item, &need_switch);
    if (need_switch == pdTRUE)
        portYIELD_FROM_ISR(need_switch);

    return osal_queue_from_isr_result_to_status(ok);
}

osal_status_t osal_queue_recv(osal_queue_t queue, void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    BaseType_t ok = xQueueReceive((QueueHandle_t)queue, item, ticks);
    return osal_wait_result_to_status(ok, timeout_ms);
}

osal_status_t osal_queue_recv_from_isr(osal_queue_t queue, void* item)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_isr_context())
        return OSAL_INVALID;

    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xQueueReceiveFromISR((QueueHandle_t)queue, item, &need_switch);
    if (need_switch == pdTRUE)
        portYIELD_FROM_ISR(need_switch);

    return osal_queue_from_isr_result_to_status(ok);
}

osal_status_t osal_queue_peek(osal_queue_t queue, void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    BaseType_t ok = xQueuePeek((QueueHandle_t)queue, item, ticks);
    return osal_wait_result_to_status(ok, timeout_ms);
}

osal_status_t osal_queue_peek_from_isr(osal_queue_t queue, void* item)
{
    if (!queue || !item)
        return OSAL_INVALID;
    if (!osal_queue_check_isr_context())
        return OSAL_INVALID;

    BaseType_t ok = xQueuePeekFromISR((QueueHandle_t)queue, item);
    return osal_queue_from_isr_result_to_status(ok);
}

osal_status_t osal_queue_reset(osal_queue_t queue)
{
    if (!queue)
        return OSAL_INVALID;
    if (!osal_queue_check_task_context())
        return OSAL_INVALID;

    return (xQueueReset((QueueHandle_t)queue) == pdPASS) ? OSAL_OK : OSAL_INTERNAL;
}

osal_status_t osal_queue_messages_waiting(osal_queue_t queue, uint32_t* out_count)
{
    if (!queue || !out_count)
        return OSAL_INVALID;

    if (osal_is_in_isr())
    {
        *out_count = (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)queue);
        return OSAL_OK;
    }

    *out_count = (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)queue);
    return OSAL_OK;
}

osal_status_t osal_queue_spaces_available(osal_queue_t queue, uint32_t* out_count)
{
    if (!queue || !out_count)
        return OSAL_INVALID;

    if (osal_is_in_isr())
    {
        UBaseType_t queue_length = uxQueueGetQueueLength((QueueHandle_t)queue);
        UBaseType_t waiting = uxQueueMessagesWaitingFromISR((QueueHandle_t)queue);
        if (waiting > queue_length)
        {
            OSAL_ASSERT(waiting <= queue_length);
            return OSAL_INTERNAL;
        }

        *out_count = (uint32_t)(queue_length - waiting);
        return OSAL_OK;
    }

    *out_count = (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)queue);
    return OSAL_OK;
}
