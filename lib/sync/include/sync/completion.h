#ifndef __AWLF_SYNC_COMPLETION_H__
#define __AWLF_SYNC_COMPLETION_H__

#include <stddef.h>
#include <stdint.h>

#include "core/aw_def.h"
#include "osal/osal_sem.h"
#include "osal/osal_thread.h"

/* completion 语义：单等待者 one-shot 完成事件 */
typedef enum
{
    COMP_INIT = 0,
    COMP_WAIT,
    COMP_DONE
} comp_status_e;

typedef struct Completion* Completion_t;
typedef struct Completion
{
    osal_sem_t sem;
    osal_thread_t wait_thread;
    volatile comp_status_e status;
} Completion_s;

AwlfRet_e completion_init(Completion_t completion);
void completion_deinit(Completion_t completion);

AwlfRet_e completion_wait(Completion_t completion, size_t timeout_ms);
AwlfRet_e completion_done(Completion_t completion);

#endif /* __AWLF_SYNC_COMPLETION_H__ */
