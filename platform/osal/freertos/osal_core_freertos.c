#include "osal/osal_core.h"

#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define OSAL_STATIC_ASSERT(cond, msg) _Static_assert((cond), #msg)
#else
#define OSAL_STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond) ? 1 : -1]
#endif

OSAL_STATIC_ASSERT((OSAL_PRIORITY_MAX == configMAX_PRIORITIES), osal_priority_max_mismatch);
OSAL_STATIC_ASSERT((OSAL_TASK_NAME_MAX == configMAX_TASK_NAME_LEN), osal_task_name_max_mismatch);
OSAL_STATIC_ASSERT((OSAL_STACK_WORD_BYTES == sizeof(StackType_t)), osal_stack_word_bytes_mismatch);
OSAL_STATIC_ASSERT((OSAL_WAIT_FOREVER == portMAX_DELAY), osal_wait_forever_mismatch);

int osal_in_isr(void)
{
    return (xPortIsInsideInterrupt() == pdTRUE) ? 1 : 0;
}

void osal_critical_enter(void)
{
    taskENTER_CRITICAL();
}

void osal_critical_exit(void)
{
    taskEXIT_CRITICAL();
}

uint32_t osal_irq_save(void)
{
    if (osal_in_isr())
    {
        UBaseType_t mask = taskENTER_CRITICAL_FROM_ISR();
        return 0x80000000u | (uint32_t)mask;
    }
    taskENTER_CRITICAL();
    return 0u;
}

void osal_irq_restore(uint32_t state)
{
    if ((state & 0x80000000u) != 0u)
    {
        UBaseType_t mask = (UBaseType_t)(state & 0x7FFFFFFFu);
        taskEXIT_CRITICAL_FROM_ISR(mask);
        return;
    }
    taskEXIT_CRITICAL();
}

void* osal_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void osal_free(void* ptr)
{
    vPortFree(ptr);
}
