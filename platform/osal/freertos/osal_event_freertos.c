#include "osal/osal_event.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "osal_time_freertos.h"

int osal_event_create(osal_event_t* event)
{
    if (!event)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;

    EventGroupHandle_t handle = xEventGroupCreate();
    if (!handle)
        return OSAL_ERR_NOMEM;

    *event = (osal_event_t)handle;
    return OSAL_OK;
}

int osal_event_delete(osal_event_t event)
{
    if (!event)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;

    vEventGroupDelete((EventGroupHandle_t)event);
    return OSAL_OK;
}

int osal_event_set(osal_event_t event, uint32_t flags)
{
    if (!event)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;

    (void)xEventGroupSetBits((EventGroupHandle_t)event, (EventBits_t)flags);
    return OSAL_OK;
}

int osal_event_set_isr(osal_event_t event, uint32_t flags)
{
    if (!event)
        return OSAL_ERR_PARAM;

    BaseType_t need_switch = pdFALSE;
    BaseType_t ok = xEventGroupSetBitsFromISR((EventGroupHandle_t)event, (EventBits_t)flags, &need_switch);
    if (need_switch)
        portYIELD_FROM_ISR(need_switch);
    return (ok == pdPASS) ? OSAL_OK : OSAL_ERR;
}

int osal_event_clear(osal_event_t event, uint32_t flags)
{
    if (!event)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;

    (void)xEventGroupClearBits((EventGroupHandle_t)event, (EventBits_t)flags);
    return OSAL_OK;
}

int osal_event_wait(osal_event_t event, uint32_t wait_mask, uint32_t* out_value, uint32_t timeout_ms, uint32_t options)
{
    if (!event || wait_mask == 0U)
        return OSAL_ERR_PARAM;
    if (osal_in_isr())
        return OSAL_ERR_CTX;

    const BaseType_t clear_on_exit = ((options & OSAL_EVENT_OPT_NO_CLEAR) != 0U) ? pdFALSE : pdTRUE;
    const BaseType_t wait_for_all = ((options & OSAL_EVENT_OPT_WAIT_ALL) != 0U) ? pdTRUE : pdFALSE;

    TickType_t ticks = osal_ms_to_ticks(timeout_ms);
    EventBits_t bits = xEventGroupWaitBits((EventGroupHandle_t)event, (EventBits_t)wait_mask, clear_on_exit, wait_for_all, ticks);

    if (out_value)
        *out_value = (uint32_t)bits;

    if (wait_for_all)
        return ((bits & (EventBits_t)wait_mask) == (EventBits_t)wait_mask) ? OSAL_OK : OSAL_ERR_TIMEOUT;
    return ((bits & (EventBits_t)wait_mask) != 0U) ? OSAL_OK : OSAL_ERR_TIMEOUT;
}
