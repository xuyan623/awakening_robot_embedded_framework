#include "osal/osal_queue.h"

#include "FreeRTOS.h"
#include "osal_time_freertos.h"
#include "queue.h"

typedef struct
{
    QueueHandle_t handle;
    uint32_t length;
} osal_queue_entry_t;

static osal_queue_entry_t g_queue_registry[OSAL_QUEUE_REGISTRY_MAX];

static uint32_t _queue_registry_length(QueueHandle_t handle)
{
    uint32_t length = 0;
    osal_critical_enter();
    for (uint32_t i = 0; i < OSAL_QUEUE_REGISTRY_MAX; i++)
    {
        if (g_queue_registry[i].handle == handle)
        {
            length = g_queue_registry[i].length;
            break;
        }
    }
    osal_critical_exit();
    return length;
}

static void _queue_registry_add(QueueHandle_t handle, uint32_t length)
{
    osal_critical_enter();
    for (uint32_t i = 0; i < OSAL_QUEUE_REGISTRY_MAX; i++)
    {
        if (g_queue_registry[i].handle == NULL)
        {
            g_queue_registry[i].handle = handle;
            g_queue_registry[i].length = length;
            break;
        }
    }
    osal_critical_exit();
}

static void _queue_registry_remove(QueueHandle_t handle)
{
    osal_critical_enter();
    for (uint32_t i = 0; i < OSAL_QUEUE_REGISTRY_MAX; i++)
    {
        if (g_queue_registry[i].handle == handle)
        {
            g_queue_registry[i].handle = NULL;
            g_queue_registry[i].length = 0;
            break;
        }
    }
    osal_critical_exit();
}

int osal_queue_create(osal_queue_t* queue, uint32_t length, uint32_t item_size)
{
    if (!queue || length == 0 || item_size == 0)
        return OSAL_ERR_PARAM;
    QueueHandle_t handle = xQueueCreate((UBaseType_t)length, (UBaseType_t)item_size);
    if (!handle)
        return OSAL_ERR_NOMEM;
    _queue_registry_add(handle, length);
    *queue = (osal_queue_t)handle;
    return OSAL_OK;
}

int osal_queue_delete(osal_queue_t queue)
{
    if (!queue)
        return OSAL_ERR_PARAM;
    _queue_registry_remove((QueueHandle_t)queue);
    vQueueDelete((QueueHandle_t)queue);
    return OSAL_OK;
}

int osal_queue_send(osal_queue_t queue, const void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    return (xQueueSend((QueueHandle_t)queue, item, ticks) == pdPASS) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_queue_send_isr(osal_queue_t queue, const void* item)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xQueueSendFromISR((QueueHandle_t)queue, item, &need_switch);
    if (need_switch)
        portYIELD_FROM_ISR(need_switch);
    return (ok == pdPASS) ? OSAL_OK : OSAL_ERR;
}

int osal_queue_recv(osal_queue_t queue, void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    return (xQueueReceive((QueueHandle_t)queue, item, ticks) == pdPASS) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_queue_recv_isr(osal_queue_t queue, void* item)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xQueueReceiveFromISR((QueueHandle_t)queue, item, &need_switch);
    if (need_switch)
        portYIELD_FROM_ISR(need_switch);
    return (ok == pdPASS) ? OSAL_OK : OSAL_ERR;
}

int osal_queue_peek(osal_queue_t queue, void* item, uint32_t timeout_ms)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    OSAL_ASSERT_IN_TASK();
    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    return (xQueuePeek((QueueHandle_t)queue, item, ticks) == pdPASS) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}

int osal_queue_peek_isr(osal_queue_t queue, void* item)
{
    if (!queue || !item)
        return OSAL_ERR_PARAM;
    BaseType_t ok = xQueuePeekFromISR((QueueHandle_t)queue, item);
    return (ok == pdPASS) ? OSAL_OK : OSAL_ERR;
}

int osal_queue_reset(osal_queue_t queue)
{
    if (!queue)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;
    xQueueReset((QueueHandle_t)queue);
    return OSAL_OK;
}

uint32_t osal_queue_messages_waiting(osal_queue_t queue)
{
    if (!queue)
        return 0;
    if (osal_in_isr())
        return (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)queue);
    return (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)queue);
}

uint32_t osal_queue_spaces_available(osal_queue_t queue)
{
    if (!queue)
        return 0;
    if (osal_in_isr())
    {
        uint32_t length = _queue_registry_length((QueueHandle_t)queue);
        uint32_t waiting = (uint32_t)uxQueueMessagesWaitingFromISR((QueueHandle_t)queue);
        if (length < waiting)
            return 0;
        return length - waiting;
    }
    return (uint32_t)uxQueueSpacesAvailable((QueueHandle_t)queue);
}
