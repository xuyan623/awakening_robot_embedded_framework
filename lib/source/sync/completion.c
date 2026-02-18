#include "sync/completion.h"

#include "core/aw_config.h"
#include "osal/osal_core.h"
#include "osal/osal_port.h"

#if (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)
/*
 * 加速后端的实现位于：
 *   platform/sync/<os>/completion_notify.c
 * 该文件应仅在 对应 OS 构建且启用宏时参与编译。
 */
AwlfRet_e completion_accel_init(Completion_t completion);
void completion_accel_deinit(Completion_t completion);
AwlfRet_e completion_accel_wait(Completion_t completion, size_t timeout_ms);
AwlfRet_e completion_accel_done(Completion_t completion);
#endif

static uint32_t completion_timeout_to_osal_ms(size_t timeout_ms)
{
    if (timeout_ms >= (size_t)0xFFFFFFFFu)
        return OSAL_WAIT_FOREVER;
    return (uint32_t)timeout_ms;
}

static void completion_sem_drain(osal_sem_t sem)
{
    if (!sem)
        return;
    while (osal_sem_wait(sem, 0u) == OSAL_OK)
    {
    }
}

static AwlfRet_e completion_sem_init(Completion_t completion)
{
    if (!completion)
        return AWLF_ERROR_NULL;

    if (!completion->sem)
    {
        if (osal_sem_create(&completion->sem, 1u, 0u) != OSAL_OK)
            return AWLF_ERROR_MEMORY;
    }

    completion_sem_drain(completion->sem);
    completion->wait_thread = NULL;
    completion->status = COMP_INIT;
    return AWLF_OK;
}

static void completion_sem_deinit(Completion_t completion)
{
    if (!completion)
        return;

    if (completion->sem)
    {
        (void)osal_sem_delete(completion->sem);
        completion->sem = NULL;
    }

    completion->wait_thread = NULL;
    completion->status = COMP_INIT;
}

static AwlfRet_e completion_sem_wait_one_shot(Completion_t completion, size_t timeout_ms)
{
    if (!completion || !completion->sem)
        return AWLF_ERROR_PARAM;
    if (osal_is_in_isr())
        return AWLF_ERROR_PARAM;

    osal_thread_t self = osal_thread_self();
    if (!self)
        return AWLF_ERROR;

    osal_irq_lock_task();
    if (completion->status == COMP_WAIT)
    {
        osal_irq_unlock_task();
        return AWLF_ERROR_BUSY;
    }
    if (completion->status == COMP_DONE)
    {
        completion->status = COMP_INIT;
        completion->wait_thread = NULL;
        osal_irq_unlock_task();
        completion_sem_drain(completion->sem);
        return AWLF_OK;
    }
    if (timeout_ms == 0U)
    {
        osal_irq_unlock_task();
        return AWLF_ERROR_TIMEOUT;
    }
    if (completion->wait_thread != NULL)
    {
        osal_irq_unlock_task();
        return AWLF_ERROR_BUSY;
    }

    completion->wait_thread = self;
    completion->status = COMP_WAIT;
    osal_irq_unlock_task();

    uint32_t osal_timeout_ms = completion_timeout_to_osal_ms(timeout_ms);
    int wait_result = osal_sem_wait(completion->sem, osal_timeout_ms);
    AwlfRet_e ret = (wait_result == OSAL_OK) ? AWLF_OK : AWLF_ERROR_TIMEOUT;

    osal_irq_lock_task();
    if (ret != AWLF_OK && completion->status != COMP_DONE)
    {
        completion->wait_thread = NULL;
        completion->status = COMP_INIT;
        osal_irq_unlock_task();
        return AWLF_ERROR_TIMEOUT;
    }

    completion->wait_thread = NULL;
    completion->status = COMP_INIT;
    osal_irq_unlock_task();
    completion_sem_drain(completion->sem);
    return AWLF_OK;
}

static AwlfRet_e completion_sem_done_one_shot(Completion_t completion)
{
    if (!completion || !completion->sem)
        return AWLF_ERROR_PARAM;

    int in_isr = osal_is_in_isr();
    osal_irq_isr_state_t isr_state = 0u;
    AwlfRet_e ret;

    if (in_isr)
        isr_state = osal_irq_lock_from_isr();
    else
        osal_irq_lock_task();

    switch (completion->status)
    {
    case COMP_INIT:
        completion->status = COMP_DONE;
        if (osal_is_in_isr())
            (void)osal_sem_post_from_isr(completion->sem);
        else
            (void)osal_sem_post(completion->sem);
        ret = AWLF_OK;
        break;

    case COMP_WAIT:
        if (completion->wait_thread != NULL)
        {
            if (osal_is_in_isr())
                (void)osal_sem_post_from_isr(completion->sem);
            else
                (void)osal_sem_post(completion->sem);
            completion->wait_thread = NULL;
            completion->status = COMP_DONE;
            ret = AWLF_OK;
        }
        else
        {
            ret = AWLF_ERROR_EMPTY;
        }
        break;

    case COMP_DONE:
        ret = AWLF_ERROR_BUSY;
        break;

    default:
        ret = AWLF_ERROR;
        break;
    }

    if (in_isr)
        osal_irq_unlock_from_isr(isr_state);
    else
        osal_irq_unlock_task();
    return ret;
}

AwlfRet_e completion_init(Completion_t completion)
{
#if (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)
    return completion_accel_init(completion);
#else
    return completion_sem_init(completion);
#endif
}

void completion_deinit(Completion_t completion)
{
#if (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)
    completion_accel_deinit(completion);
#else
    completion_sem_deinit(completion);
#endif
}

AwlfRet_e completion_wait(Completion_t completion, size_t timeout_ms)
{
#if (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)
    return completion_accel_wait(completion, timeout_ms);
#else
    return completion_sem_wait_one_shot(completion, timeout_ms);
#endif
}

AwlfRet_e completion_done(Completion_t completion)
{
#if (AWLF_SYNC_ACCEL) && (AWLF_SYNC_ACCEL_COMPLETION)
    return completion_accel_done(completion);
#else
    return completion_sem_done_one_shot(completion);
#endif
}



