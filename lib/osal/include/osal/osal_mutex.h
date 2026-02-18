#ifndef __AWLF_OSAL_MUTEX_H__
#define __AWLF_OSAL_MUTEX_H__

#include <stdint.h>

#include "osal_core.h"

typedef void* osal_mutex_t;

/**
 * @brief 创建互斥锁（线程上下文）
 * @param mutex 输出互斥锁句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID/OSAL_NO_RESOURCE`
 * @note 禁止在 ISR 中调用。
 */
osal_status_t osal_mutex_create(osal_mutex_t* mutex);

/**
 * @brief 删除互斥锁（线程上下文）
 * @param mutex 互斥锁句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID`
 * @note 禁止在 ISR 中调用。
 * @note 严格前置条件：调用方需确保无并发访问和无等待者。
 */
osal_status_t osal_mutex_delete(osal_mutex_t mutex);

/**
 * @brief 加锁（线程上下文，非递归语义）
 * @param mutex 互斥锁句柄
 * @param timeout_ms 超时时间（ms），可用 OSAL_WAIT_FOREVER
 * @return `OSAL_OK` 成功；失败返回 `OSAL_WOULD_BLOCK/OSAL_TIMEOUT/OSAL_INVALID/OSAL_INTERNAL`
 * @note 禁止在 ISR 中调用。
 * @note v1.0 不支持递归 mutex；同线程重复加锁按超时规则返回 `OSAL_WOULD_BLOCK/OSAL_TIMEOUT`（或无限等待）。
 */
osal_status_t osal_mutex_lock(osal_mutex_t mutex, uint32_t timeout_ms);

/**
 * @brief 解锁（线程上下文）
 * @param mutex 互斥锁句柄
 * @return `OSAL_OK` 成功；失败返回 `OSAL_INVALID`
 * @note 禁止在 ISR 中调用。
 * @note 非 owner 解锁返回 `OSAL_INVALID`。
 * @note 互斥锁应避免跨线程释放；仅持有锁的线程可执行解锁。
 */
osal_status_t osal_mutex_unlock(osal_mutex_t mutex);

#endif
